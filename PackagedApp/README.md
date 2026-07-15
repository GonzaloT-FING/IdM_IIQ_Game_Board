# IdM Trivia — paquete listo para usar

Esta carpeta contiene una copia transportable de la aplicación estable y todos sus recursos de ejecución. No requiere Node.js, `npm` ni conexión a Internet.

## Abrir la aplicación

1. Haz doble clic en `Abrir_IdM_Trivia.cmd` para abrirla en Microsoft Edge.
2. En la pantalla inicial, selecciona **Importar contenido**.
3. Elige `QuestionPacks/MujeresCiencia.idmquiz`.
4. Pulsa **Conectar** y selecciona el puerto del tablero físico cuando corresponda.

También puedes abrir manualmente `NewApp/IdM_Trivia2.html` con Microsoft Edge o Google Chrome. Esos navegadores son necesarios para Web Serial.

## Contenido

- `NewApp/IdM_Trivia2.html`: aplicación completa.
- `SVG/`: casco, tablero y mapa lógico requeridos por la interfaz.
- `QuestionPacks/MujeresCiencia.idmquiz`: paquete de preguntas más reciente, con cuatro preguntas y cuatro imágenes.
- `Firmware/`: firmware Arduino compatible con las actualizaciones de estado del tablero.
- `Instructivo Trivia.pdf`: instructivo de uso.

El paquete de preguntas no se carga automáticamente para que el anfitrión pueda reemplazarlo por otro desde **Importar contenido**.
