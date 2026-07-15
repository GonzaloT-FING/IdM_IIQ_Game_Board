# IdM IIQ Game Board

Interactive trivia board game with a browser-based game interface and an Arduino-powered physical controller/LED board.

The current development app in [NewApp/IdM_Trivia2.html](NewApp/IdM_Trivia2.html) already communicates well with the board, loads questions from JSON, and supports the configuration panel. The next phase is to turn the experience into a full game-master flow where the app tracks turns, board positions, scoring, and the logic for each square automatically.

## Nueva dirección de desarrollo: modo maestro de juego

La idea para la siguiente versión es que la pantalla inicial ya no muestre las opciones MC, TF, Fortuna o Mala suerte como modos independientes. En su lugar, el flujo será:

1. Pantalla de inicio con el botón “Comenzar Juego”.
2. Ingreso de 1 a 6 jugadores y sus nombres.
3. Visualización del tablero con todas las fichas ubicadas en la casilla inicial.
4. Avance paso a paso con la flecha derecha, donde cada paso representa una acción del turno.
5. En el inicio del turno, el sistema inicia con el lanzamiento del dado.
6. Luego se mueve la ficha del jugador en el tablero según el resultado.
7. El tablero reconoce la casilla en la que cayó el jugador y activa automáticamente el modo correspondiente: MC, TF, Fortuna o Mala suerte.
8. Se aplica el resultado de la respuesta: puntos ganados o perdidos según corresponda.
9. Después de completar la acción, pasa el turno al siguiente jugador.
10. Si ocurre un error, el juego maestro puede volver un paso atrás y restaurar el estado anterior.
11. El juego termina cuando un jugador llega al final y se muestra una pantalla de ganador con marcador.

### Reglas que se quieren conservar

- El orden de los jugadores se respeta tal como fue ingresado.
- El app será la fuente de verdad para posiciones, turnos y puntajes.
- El tablero solo debe responder a lo que el app indique.
- El flujo debe ser muy claro para el game master y permitir avanzar o retroceder con facilidad.
- El tablero debe reflejar la forma real, el recorrido real y la ubicación de cada pieza de forma intuitiva.

### Recorrido del tablero

- Inicio común: `1 → 2 → 3 → 4`.
- Rama corta (flecha arriba): `16 → 17 → 18 → 19 → 20`.
- Rama larga (flecha abajo): `5 → 6 → 7 → 8 → 9 → 10 → 11`.
- Convergencia y final común: `12 → 13 → 14 → 15`.

### Implementación propuesta

- Preferir un tablero renderizado con SVG para mantener la forma real del recorrido y facilitar la representación precisa de casillas, colores y fichas.
- Mantener la lógica de preguntas y respuestas existente, pero integrarla en un flujo de turno automático.
- Implementar un historial de pasos para la funcionalidad de deshacer y rehacer.
- Enviar comandos al tablero desde la app para reflejar el estado del juego y sincronizar luces o animaciones si se desea después.

## Repository Contents

- `IdM_Trivia_v3_3_33_idle_random_V_at_15.html` - Single-file browser app for the trivia game.
- `LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino` - Arduino sketch for the LED strip, discrete indicator LEDs, serial command handling, and four physical buttons.
- `PregsTodos.json` - Question/content pack. Current contents: 66 multiple-choice questions, 55 true/false questions, 1 fortune card, and 1 bad-luck card.
- `QuestionPacks/` - Editable image-aware question packs, their source images, and a reusable package builder.
- `PackagedApp/` - Portable stable app with runtime assets, firmware, instructions, and the latest question pack.
- `Instructivo Trivia.pdf` - User-facing guide/instruction document.

## How The Pieces Fit Together

1. Upload `LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino` to the Arduino-compatible board.
2. Open or serve `IdM_Trivia_v3_3_33_idle_random_V_at_15.html` in Chrome or Edge.
3. Use the app's `Importar contenido` button or drag and drop to load `PregsTodos.json`.
4. Click `Conectar` in the app and select the Arduino serial port.
5. During the game, the web app sends commands over Web Serial at `115200` baud.
6. The Arduino reacts with LED animations and also sends keyboard presses `1`, `2`, `3`, and `4` when the physical buttons are pressed.

Web Serial requires Chrome/Edge and a secure context. `localhost` or HTTPS work; plain `file://` may be enough for viewing the UI but is not reliable for connecting to the board.

## Game Controls

The development app now opens in game-manager setup. During a game:

- `Right Arrow` - perform the next turn action (open the die, confirm movement, or continue).
- `Left Arrow` - undo the latest gameplay action.
- `Up Arrow` - choose the short branch when crossing case 4.
- `Down Arrow` - choose the long branch when crossing case 4.
- `1`, `2`, `3`, `4` - launch the manual die in that button's color or answer the active trivia question.

Manual die mode is the default. The die occupies most of the screen and waits for one of the four interaction buttons. Automatic mode can be enabled in `Configuración`.

The older configurable bindings are still used by the preserved trivia and dice renderers:

- `Z` - Multiple choice
- `X` - True/false
- `C` - Fortune
- `V` - Bad luck
- `D` - Dice
- `F` - Dice 2
- `1`, `2`, `3`, `4` - Answer choices
- `Space` - Return to start
- `Enter` - New item in the current category

The physical Arduino buttons also type `1`, `2`, `3`, and `4`, so the same answer logic is shared between the board and the keyboard.

## Web App Features

The HTML app includes:

- Two finish conditions: stop at the first finisher or continue until every player finishes.
- Player setup with six fixed colors: red, pink, black, blue, yellow, and green.
- Multiple-choice and true/false screens with a timer and explanation reveal.
- Fortune and bad-luck cards.
- Configurable points for multiple choice, true/false, fortune, bad luck, and the first player to finish.
- Two dice modes, including automatic and player-triggered roll behavior.
- Persistent light and dark interface themes.
- Confetti and colored feedback flashes.
- Runtime customization for timer length, progress bar size, text styles, key bindings, and dice behavior.
- Background image upload from the `Assets` panel. These uploaded images apply only to the current browser tab/session and are not saved after reload.
- JSON and image-aware `.idmquiz`/ZIP import via file picker or drag and drop.
- Web Serial connection to the Arduino.
- LED idle controls for changing strip group lengths and color sequences.

Most UI settings are saved in browser `localStorage`.

## Arduino Hardware Notes

The sketch is written for a Pro Micro / Leonardo class board using the ATmega32U4, because it uses `Keyboard.h` for USB keyboard output.

Default pin settings:

- NeoPixel data pin: `6`
- NeoPixel count: `213`
- Button input pins: `2`, `3`, `4`, `5`
- Discrete indicator LED pins: `8`, `9`, `10`, `11`
- Serial baud rate: `115200`

Required Arduino libraries:

- `Adafruit_NeoPixel`
- `Keyboard` (available for compatible ATmega32U4 boards)

## Serial Protocol

The browser app sends newline-terminated commands. The sketch recognizes:

```text
HELLO
GOODBYE
MODE:MC
MODE:TF
MODE:FORTUNE
MODE:BAD
ANS:CORRECT
ANS:WRONG
IDLE:ON
IDLE:OFF
IDLE:RESET
IDLE:GROUPS=14,8,12,...
IDLE:COLORS=V,B,G,Y,R,...
KEY:ON
KEY:OFF
BOARD:T=3;A=1;S=move-preview;H=9;P=1@4@S@2563EB,2@10@L@BE123C
BOARD:CLEAR
```

`BOARD:` is a complete browser-owned snapshot. `T` is the turn, `A` is the zero-based active-player index, `S` is the current phase, `H` lists up to two destination cases to blink, and every `P` tuple contains player id, case, branch (`S`, `L`, or `N`), and RGB color. At rest the firmware renders every case in its original color. Destination cases alternate between white and their original color; question and answer animations temporarily override the whole board and then return it to the original colors.

Current sketch behavior:

- `MODE:MC` - Blue pulse animation.
- `MODE:TF` - Green trails animation and only discrete LEDs 1 and 2 stay on.
- `MODE:FORTUNE` - Yellow twinkle animation.
- `MODE:BAD` - Red pulse animation.
- `ANS:CORRECT` - Green pulse animation and all discrete LEDs return on.
- `ANS:WRONG` - Red flash animation and all discrete LEDs return on.
- `IDLE:*` - Runtime control of the idle LED strip pattern.
- `KEY:*` - Enable or disable physical button keyboard output.

The web app also emits `MODE:DICE`, `MODE:DICE2`, `DICE:ROLL:<n>`, and `DICE2:ROLL:<n>`. The current Arduino sketch ignores those commands, so dice mode is visual in the browser unless LED behavior is added later.

## Question JSON Format

The question file can use keys such as `mc`, `tf`, `fortune`, and `bad`.

Example:

```json
{
  "mc": [
    {
      "id": "Q1",
      "prompt": "Question text",
      "options": ["Option A", "Option B", "Option C", "Option D"],
      "correctIndex": 1,
      "explanation": "Optional explanation after the answer.",
      "image": {
        "path": "images/Q1.webp",
        "alt": "Accessible description of the image"
      }
    }
  ],
  "tf": [
    {
      "id": "T1",
      "prompt": "Statement text",
      "answerTrue": true,
      "explanation": "Optional explanation after the answer."
    }
  ],
  "fortune": [
    {
      "id": "F1",
      "title": "Fortune",
      "text": "Card text"
    }
  ],
  "bad": [
    {
      "id": "B1",
      "title": "Bad luck",
      "text": "Card text"
    }
  ]
}
```

`correctIndex` is zero-based. The importer also attempts to adjust a one-based value when it equals the number of options.

The optional `image` object is supported by multiple-choice questions, true/false questions, fortune cards, and bad-luck cards. `path` is relative to `questions.json`; `alt` and `caption` are optional. `fit` may be `contain` (default) or `cover`.

## Image-Aware Question Packages

An `.idmquiz` file is a standard ZIP archive with this layout:

```text
questions.json
images/
  Q1.webp
  Q2.jpg
```

The importer also accepts the `.zip` extension. It prefers an explicit `image.path`; if one is not present, it automatically looks for `images/<question-id>.webp`, `.jpg`, `.jpeg`, `.png`, or `.gif`. Plain JSON files remain supported, but local image paths require a ZIP/`.idmquiz` package because the browser cannot read sibling files selected through a JSON file picker.

The example pack is available as:

- `QuestionPacks/MujeresCiencia/questions.json` - editable two-question source.
- `QuestionPacks/MujeresCiencia/images/` - the images named after their question IDs.
- `QuestionPacks/MujeresCiencia.idmquiz` - ready-to-import package.

To rebuild a package in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\QuestionPacks\build-pack.ps1 `
  -SourceDirectory .\QuestionPacks\MujeresCiencia `
  -OutputFile .\QuestionPacks\MujeresCiencia.idmquiz
```

## Packaged App

`PackagedApp/` is the copy-ready distribution for the game computer. Open `PackagedApp/Abrir_IdM_Trivia.cmd`, import `PackagedApp/QuestionPacks/MujeresCiencia.idmquiz`, and connect the board from Microsoft Edge. The packaged browser app has no Node.js or Internet dependency.

Refresh the distribution after changing the app, runtime SVG files, firmware, guide, or question pack:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-packaged-app.ps1
```

## Development Notes

- Keep the `.html`, `.json`, and `.ino` files as UTF-8 text.
- The app is currently a single HTML file with embedded CSS and JavaScript, which makes it easy to copy to the game machine but harder to maintain as it grows.
- The PDF is useful as a player/facilitator guide, but the source of truth for the technical behavior is currently the HTML, JSON, and Arduino sketch.

## Development Tooling

The repository uses Node.js LTS with Vite for the local development server, Playwright for browser testing in installed Microsoft Edge, and `fflate` for future ZIP/package handling improvements. Install dependencies once with:

```powershell
npm install
```

Start the local app server:

```powershell
npm run dev
```

Then open `http://127.0.0.1:4173/NewApp/IdM_Trivia2.html`. Run the browser smoke tests with:

```powershell
npm test
```

Playwright test output, Vite build output, and `node_modules` are excluded from Git. The game itself still runs offline and does not require Node.js on the presentation computer unless the Vite development server is being used.
