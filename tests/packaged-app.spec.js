import { expect, test } from "@playwright/test";
import path from "node:path";

test("packaged app contains its runtime assets and latest question pack", async ({ page, request }) => {
  const helmet = await request.get("/PackagedApp/SVG/Casco.svg");
  expect(helmet.ok()).toBe(true);

  await page.goto("/PackagedApp/NewApp/IdM_Trivia2.html");
  await expect(page.getByRole("heading", { name: "Preparar partida" })).toBeVisible();

  await page.setInputFiles(
    "#importInput",
    path.resolve("PackagedApp/QuestionPacks/MujeresCiencia.idmquiz")
  );
  await expect(page.locator("#toastTitle")).toHaveText("Paquete importado");
  await expect(page.locator("#toastDetails")).toContainText("OM: 2");
  await expect(page.locator("#toastDetails")).toContainText("VF: 2");
  await expect(page.locator("#toastDetails")).toContainText("Imágenes: 4");

  await page.getByRole("button", { name: "Comenzar juego" }).click();
  await expect(page.locator(".turn-icon image")).toHaveAttribute("href", "../SVG/Casco.svg");
});
