import { expect, test } from "@playwright/test";
import path from "node:path";

test("image package, player helmet, die viewport, and question layout work together", async ({ page }) => {
  await page.goto("/NewApp/IdM_Trivia2.html");
  await expect(page.getByRole("heading", { name: "Preparar partida" })).toBeVisible();
  const firstPalette = page.getByRole("radiogroup", { name: "Color del jugador 1" });
  await expect(firstPalette.getByRole("radio")).toHaveCount(6);
  await firstPalette.getByRole("radio", { name: "Rosa" }).click();
  await expect(firstPalette.getByRole("radio", { name: "Rosa" })).toHaveAttribute("aria-checked", "true");
  await expect(page.locator("select.player-color")).toHaveCount(0);

  const packagePath = path.resolve("QuestionPacks/MujeresCiencia.idmquiz");
  await page.setInputFiles("#importInput", packagePath);
  await expect(page.locator("#toastTitle")).toHaveText("Paquete importado");
  await expect(page.locator("#toastDetails")).toContainText("OM: 2");
  await expect(page.locator("#toastDetails")).toContainText("VF: 2");
  await expect(page.locator("#toastDetails")).toContainText("Imágenes: 4");

  await page.getByRole("button", { name: "Comenzar juego" }).click();
  await expect(page.locator(".turn-icon image")).toHaveAttribute("href", "../SVG/Casco.svg");
  expect((await page.locator(".turn-icon").boundingBox()).width).toBeLessThanOrEqual(92);
  await expect(page.locator(".scoreboard th")).toHaveText(["Jugador", "Puntos"]);
  await expect(page.locator(".scoreboard th", { hasText: "Casilla" })).toHaveCount(0);
  expect(parseFloat(await page.locator(".score-value").first().evaluate(node => getComputedStyle(node).fontSize))).toBeGreaterThanOrEqual(28);

  await page.keyboard.press("ArrowRight");
  const die = page.locator(".game-die-panel .die");
  await expect(die).toBeVisible();
  expect((await die.boundingBox()).height).toBeLessThanOrEqual(480);
  expect(await page.locator(".stage").evaluate(node => node.scrollHeight <= node.clientHeight + 1)).toBe(true);

  await page.evaluate(() => {
    let sequence = 0;
    Math.random = () => [0.2, 0.4, 0.6, 0.8][sequence++ % 4];
    setTimeout(() => { Math.random = () => 0; }, 375);
  });
  await page.keyboard.press("1");
  await expect(page.getByRole("button", { name: "Mover a casilla 2" })).toBeVisible({ timeout: 3_000 });
  await page.keyboard.press("ArrowRight");

  await expect(page.locator(".question")).toContainText("Marie Curie");
  const image = page.locator(".question-media img");
  await expect(image).toBeVisible();
  await expect(image).toHaveAttribute("alt", "Retrato de Marie Curie");
  expect(await image.evaluate(node => node.complete && node.naturalWidth > 0)).toBe(true);

  const imageBox = await page.locator(".question-media").boundingBox();
  const questionBox = await page.locator(".question-copy").boundingBox();
  expect(imageBox.x + imageBox.width).toBeLessThan(questionBox.x);
  expect(await page.locator(".stage").evaluate(node => node.scrollHeight <= node.clientHeight + 1)).toBe(true);
});

test("true-or-false image question resolves the configured answer", async ({ page }) => {
  await page.goto("/NewApp/IdM_Trivia2.html");
  await page.setInputFiles("#importInput", path.resolve("QuestionPacks/MujeresCiencia.idmquiz"));
  await expect(page.locator("#toastDetails")).toContainText("VF: 2");

  await page.getByRole("button", { name: "Comenzar juego" }).click();
  await page.keyboard.press("ArrowRight");
  await page.evaluate(() => {
    let sequence = 0;
    Math.random = () => [0.4, 0.6, 0.8][sequence++ % 3];
    setTimeout(() => { Math.random = () => 0.25; }, 375);
  });
  await page.keyboard.press("1");
  await expect(page.getByRole("button", { name: "Mover a casilla 3" })).toBeVisible({ timeout: 3_000 });
  await page.keyboard.press("ArrowRight");

  await expect(page.locator(".question")).toContainText("Ada Lovelace");
  await expect(page.locator(".question-media img")).toHaveAttribute("alt", "Retrato de Ada Lovelace");
  await expect(page.locator(".answer")).toHaveCount(2);
  await page.keyboard.press("1");
  await expect(page.locator(".panel > .head .status")).toContainText("Correcto");
  await expect(page.locator(".explain")).toContainText("máquina analítica de Charles Babbage");
});

async function rollGameDie(page, result) {
  await page.keyboard.press("ArrowRight");
  await expect(page.locator(".game-die-panel")).toBeVisible();
  await page.evaluate(target => {
    let sequence = 0;
    Math.random = () => [0.2, 0.4, 0.6, 0.8][sequence++ % 4];
    setTimeout(() => { Math.random = () => (target - 0.5) / 6; }, 375);
  }, result);
  await page.keyboard.press("1");
}

test("the highest score wins even when another player reaches the finish", async ({ page }) => {
  await page.addInitScript(() => {
    localStorage.setItem("idm_trivia2_settings_v1", JSON.stringify({ scoring: { firstFinish: 0, bad: 0 } }));
  });
  await page.goto("/NewApp/IdM_Trivia2.html");
  await page.getByLabel("Nombre del jugador 1").fill("Finalista");
  await page.getByLabel("Nombre del jugador 2").fill("Puntaje");
  await page.getByRole("button", { name: "Comenzar juego" }).click();

  await rollGameDie(page, 6);
  await expect(page.getByRole("button", { name: "↑ Rama corta" })).toBeVisible({ timeout: 3_000 });
  await page.getByRole("button", { name: "↑ Rama corta" }).click();
  await expect(page.getByRole("button", { name: "Mover a casilla 18" })).toBeVisible();
  await page.keyboard.press("ArrowRight");
  await expect(page.getByRole("heading", { name: "Mala suerte" })).toBeVisible();
  await page.keyboard.press("ArrowRight");

  await rollGameDie(page, 1);
  await expect(page.getByRole("button", { name: "Mover a casilla 2" })).toBeVisible({ timeout: 3_000 });
  await page.keyboard.press("ArrowRight");
  await page.getByRole("button", { name: /Hydrogen/ }).click();
  await page.keyboard.press("ArrowRight");

  await rollGameDie(page, 6);
  await expect(page.getByRole("button", { name: "Mover a casilla 15" })).toBeVisible({ timeout: 3_000 });
  await page.keyboard.press("ArrowRight");

  await expect(page.getByRole("heading", { name: "¡Ganó Puntaje!" })).toBeVisible();
  await expect(page.getByText("Mayor puntaje de la partida")).toBeVisible();
  await expect(page.locator(".winner .question")).toHaveText("1 puntos");
  await expect(page.locator(".winner-token")).toHaveAttribute("title", "Puntaje");
});
