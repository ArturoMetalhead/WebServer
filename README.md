# WebServer

Integrantes:
Diana Laura Pérez Trujillo C312
Carlos Arturo Pérez Cabrera C312
Alejandro Camacho Pérez C312

Resumen del proyecto:
Este programa es un servidor web que utiliza el protocolo HTTP para servir archivos y directorios a través de la red. El servidor está diseñado para recibir conexiones de clientes a través de un socket, parsear los mensajes HTTP que recibe, y responder con los recursos solicitados en el mensaje HTTP.
El servidor se ejecuta en un bucle principal que espera eventos en los sockets utilizando la función poll(). Cuando se detecta un evento, se comprueba si es una nueva conexión o una solicitud en un socket existente. Si es una nueva conexión, se acepta la conexión y se añade el nuevo socket al array de estructuras pollfd. Si es una solicitud en un socket existente, se lee el mensaje HTTP y se parsea para obtener el recurso solicitado. Dependiendo del recurso solicitado (un archivo o un directorio,los cuales se van a poder diferenciar por el uso de un "/" al final del nombre de los directorios para lograr mejor claridad), el servidor responde con el contenido apropiado.
El programa también incluye varias funciones auxiliares que se utilizan en el manejo de los recursos solicitados. Por ejemplo, la función send_file() se utiliza para enviar el contenido de un archivo a través del socket, mientras que la función send_directory() se utiliza para enviar el contenido de un directorio. También es capaz de manejar múltiples solicitudes simultáneamente utilizando el modelo de procesamiento por fork.