# Codex implementation instructions

## Goal
Implement the full IdM board-game experience in this repository by extending the existing browser app and Arduino firmware. The browser app should become the source of truth for game state, while the board SVG and hardware reflect that state.

## Current baseline
- The web app is in [NewApp/IdM_Trivia2.html](NewApp/IdM_Trivia2.html).
- The Arduino firmware is in [LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino](LEDStrip_Animations_Serial_v1g_DiscreteLEDs_Flash6_fix3.ino).
- The board artwork is in [SVG/Tablero.svg](SVG/Tablero.svg).
- The logical board mapping is in [SVG/board-map.json](SVG/board-map.json).
- The product direction is described in [README.md](README.md).

## Core product requirements
1. Replace the current standalone trivia entry flow with a game-manager flow.
2. Add player setup with names and colors.
3. Add turn progression for the active player.
4. Add a die roll and movement flow.
5. Add a branch choice at case 4 controlled by the game host using up/down input.
6. When a player lands on a square, the app should automatically launch the corresponding trivia mode.
7. Keep the existing MC, TF, Fortune, and Bad Luck question content and reuse those screens.
8. Add scoring, undo, a winner screen, and a scoreboard.
9. Render the board from app state so the SVG highlights the current positions and path structure.
10. Send board-state updates to the Arduino so the physical board can reflect the same game state.

## Design rules
- The browser app is authoritative. It owns players, positions, scores, active turn, branch choices, and question flow.
- The SVG board is visual only. It should react to app state rather than manage state independently.
- Keep the existing trivia experience intact, but make it a sub-step inside the board-game loop.
- The board should reflect the physical board shape and real path progression, including the branch at case 4.
- Preserve the current Spanish-style UI and existing question formats unless a clear improvement is needed.

## Board model requirements
- Use the existing board mapping in [SVG/board-map.json](SVG/board-map.json) as the canonical logical path definition.
- Case 4 must support a host-driven branch decision:
  - up = short branch
  - down = long branch
- The app should store the chosen branch for the active player and use it when movement continues.
- The board rendering should highlight the current square for each player and reflect the selected path.

## Gameplay behavior
- Start from a setup screen and begin a game.
- Each turn should allow the game host to roll the die and advance the active player.
- When the player lands on a square, launch the appropriate trivia mode automatically.
- After the trivia step resolves, return to the board and continue the turn.
- Undo should revert the most recent gameplay action, including movement and trivia resolution if feasible.
- The game should end when a player reaches the final square, and show a winner screen.
- The scoreboard should show players, positions, and scores.

## Serial / hardware requirements
- Extend the existing serial protocol so the browser app can send a full board-state snapshot to the Arduino.
- The firmware should accept a structured board-state message and update its board/LED behavior accordingly.
- Keep the existing animation and answer-feedback behavior where appropriate.
- Do not break the current serial commands used by the trivia flow.

## Implementation guidance
- Reuse the current question rendering logic in [NewApp/IdM_Trivia2.html](NewApp/IdM_Trivia2.html) instead of replacing it wholesale.
- Keep the app logic organized in a single state object and render the UI from that state.
- Add any new UI screens as simple, focused views rather than separate large apps.
- Prefer incremental changes over a rewrite.
- Preserve compatibility with the current imported question JSON structure.

## Acceptance criteria
- The app can start a game with multiple players.
- The host can advance turns and move players around the board.
- Case 4 lets the host choose the short or long path.
- Landing on a square launches the matching trivia mode automatically.
- Scoring, undo, winner screen, and scoreboard all work.
- The SVG board reflects the active game state.
- The Arduino receives board-state updates and changes behavior accordingly.
