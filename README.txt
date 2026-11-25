SMARTBATTERY ANDROID + WOKWI - PAQUETE AJUSTADO

IMPORTANTE
La aplicación es una PWA (aplicación web instalable en Android), no un APK nativo.
La conexión no se hace directamente entre el teléfono y la ventana de Wokwi:
ambos se comunican mediante el mismo broker MQTT público de demostración.

1) PREPARAR WOKWI
1. Entra en https://wokwi.com/ y crea un proyecto ESP32.
2. Abre Libraries y agrega PubSubClient.
3. Abre sketch.ino y reemplaza su contenido por el archivo wokwi_sketch.ino.
4. Verifica que DEVICE_ID sea "demo01" y DEMO_MODE sea true.
5. Inicia la simulación.
6. Abre el Serial Monitor. Debes ver que se conecta a WiFi y MQTT y publica
   telemetría cada 15 segundos.

Para esta prueba no necesitas conectar una batería real. El valor 12.65 V
es ilustrativo y fijo.

2) PUBLICAR LA APP PARA ANDROID
La app necesita una dirección HTTPS para que Chrome permita instalarla.
Puedes usar GitHub Pages:
1. Crea un repositorio en GitHub.
2. Sube index.html, manifest.webmanifest y sw.js a la raíz del repositorio.
3. En Settings > Pages, activa el despliegue desde la rama principal (root).
4. Espera la URL HTTPS que entrega GitHub Pages.
5. Abre esa URL en Chrome Android.
6. Menú ⋮ > Instalar aplicación (o Agregar a pantalla principal).

También puedes abrir index.html en un computador para probarla, pero para
instalarla en Android se recomienda alojarla mediante HTTPS.

3) CONECTAR APP Y WOKWI
1. Deja la simulación Wokwi funcionando.
2. Abre SmartBattery en Android.
3. En "ID de dispositivo / canal" deja: demo01
4. Pulsa Conectar.
5. Debe cambiar a "Conectado" y aparecer aproximadamente 12.65 V.
6. Cambia el intervalo a 1, 3, 6 u 8 horas y pulsa Enviar.
7. En el Serial Monitor de Wokwi debe aparecer "Nuevo intervalo recibido...".
8. La app mostrará el intervalo nuevo cuando reciba la siguiente telemetría.

4) TEMAS MQTT (DEBEN COINCIDIR)
Telemetría: smartbattery/demo01/telemetry
Configuración: smartbattery/demo01/config
Broker: broker.hivemq.com
ESP32 usa MQTT TCP puerto 1883.
La app usa MQTT sobre WebSocket seguro (WSS) puerto 8884.

5) SI NO CONECTA
- Confirma que Wokwi sigue ejecutando la simulación.
- Confirma que el ID sea demo01 tanto en la app como en el sketch.
- Confirma que PubSubClient esté instalado en Wokwi.
- Revisa que el Serial Monitor diga "MQTT conectado".
- En el teléfono, confirma que Chrome tenga Internet y vuelve a cargar la app.
- Espera 10-20 segundos; el prototipo publica cada 15 segundos.
- El broker es público: puede estar temporalmente ocupado o recibir mensajes de terceros.

6) LIMITACIONES Y SEGURIDAD
- Es una demostración, no una conexión privada.
- No uses el broker público para datos reales del vehículo.
- Wokwi no simula una conexión celular 4G real. En esta prueba usa WiFi.
- El ESP32 no puede recibir órdenes durante Deep Sleep. En modo real despierta,
  mide, se conecta, publica y vuelve a dormir; las órdenes de intervalo se
  aplican cuando está despierto/conectado.
- El voltaje 12.65 V es simulado, no una medición real.
- Para montaje automotriz real se necesita divisor de tensión correctamente
  calculado, protección de entrada, fusible y convertidor DC-DC de bajo consumo.
  Nunca conectes los 12 V del vehículo directamente al GPIO34.
