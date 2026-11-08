# HaxBall NX — especificación maestra del proyecto

Última actualización: 31-10-2026
Estado: especificación inicial; antes de ampliar el código se debe actualizar este documento con los cambios verificados.

## Propósito y alcance

HaxBall NX es un juego de fútbol arcade 2D cenital para Nintendo Switch, inspirado en la sencillez de HaxBall. El objetivo inmediato es conseguir una versión v0.1 jugable y compilada en formato .nro. La primera entrega debe priorizar un partido funcional frente a los menús y modos avanzados.

Este documento conserva las decisiones verificadas y separa las propuestas, contradicciones y preguntas pendientes. El historial original no está disponible automáticamente en futuras sesiones: revisar esta especificación y el código real antes de afirmar que una función existe.

## Estado previo a v0.1 observado el 31-10-2026

- Proyecto local indicado: C:\Users\Usuario\Desktop\Cosas\apps\Github\HaxBall-NX.
- Proyecto en C con SDL2 para Switch; compilación mediante Makefile de devkitPro/libnx.
- La configuración del Makefile produce el objetivo HaxBall-NX.nro. El usuario pide generar únicamente .nro.
- source/main.c contiene un prototipo con campo cenital, un jugador azul, una pelota, rebotes en límites, colisión simple jugador-pelota, golpe con A y marcador.
- El control existente usa cruceta; no implementa movimiento analógico ni un segundo jugador/bot.
- Los dígitos del marcador actual se dibujan con barras de siete segmentos. Esto contradice la decisión visual y debe reemplazarse antes de presentar una interfaz aceptable.
- Existen source/main.c.bak, main.c.bak, HaxBall-NX_original.nro, HaxBall-NX.nro, HaxBall-NX.elf y la carpeta build/. No sobrescribir ni eliminar las copias existentes sin motivo explícito.
- La carpeta revisada no mostró metadatos de Git en su raíz; no asumir que hay control de versiones disponible.
- El usuario informa que devkitPro se trasladó a C:\Users\Usuario\Desktop\Cosas\apps\Github\devkitPro. En MSYS2 confirmó una compilación correcta al definir DEVKITPRO=/c/Users/Usuario/Desktop/Cosas/apps/Github/devkitPro, DEVKITA64=$DEVKITPRO/devkitA64 y añadir sus binarios a PATH. Esta configuración es contexto histórico comunicado por el usuario; volver a verificar antes de compilar.
- La primera compilación del v0.1 se ejecutó el 31-10-2026; tras corregir dos avisos, la recompilación finalizó sin avisos.

## Decisiones confirmadas

### Plataforma y presentación

- Plataforma: Nintendo Switch homebrew.
- Formato de entrega: .nro únicamente.
- Juego 2D con vista cenital de 90 grados y arena completa visible.
- Resolución lógica propuesta en la conversación: 1280 × 720; confirmar la adaptación real al modo de pantalla de Switch durante la implementación.
- Jugadores representados como círculos: azul y rojo.
- Se descartan todos los mockups e imágenes anteriores como referencia visual.
- Prohibido usar tipografía de reloj digital de siete segmentos, letras construidas con rectángulos o texto pixelado.
- Usar fuentes reales, suaves y legibles. Preferencia expresada: Montserrat Bold para títulos/marcador y Montserrat Medium para menús/opciones. Para v0.1 se eligió una fuente real Arial Bold rasterizada con antialias y embebida, ya que SDL2_ttf no está disponible en el entorno revisado. La preferencia por Montserrat queda pendiente para una etapa posterior. El marcador y los nombres deben ser dinámicos y legibles.

### Partido y controles

- Movimiento analógico de 360 grados.
- Botón A golpea/dispara una vez por pulsación, sin repetición automática mientras se mantiene pulsado.
- No hay sprint.
- Colisiones físicas entre jugadores y balón.
- El balón tiene velocidad, fricción y rebotes.
- Se permiten goles en propia puerta.
- Tras un gol, saque desde el centro y actualización del marcador.
- Cuenta atrás 3-2-1 antes de iniciar/reanudar el partido.
- Si el empate continúa al acabar el tiempo, se juega gol de oro.
- Resultado final y textos del partido deben adaptarse al estado real del juego.
- Si el balón o la partida quedan atascados, se debe detectar y reiniciar al centro.

### Modos y opciones previstos

- Partidos de 1v1 hasta 4v4, combinando mandos locales y bots.
- Selección de bandos/equipos directamente desde «Jugar», con presentación inspirada en la selección de equipos de FIFA.
- Cada humano confirma su propio estado «Listo».
- Capitanes, nombres editables y tamaño de los equipos.
- Bots con dificultades fácil, normal y difícil.
- Estadios: clásico, fútbol sala, hockey y balonmano, con variantes.
- Pausa condicionada a la posesión; pausa automática si se desconecta un mando.
- Gestionar reconexión y sustitución tras desconexión.
- Revancha mediante votos de los humanos conectados: se exige al menos la mitad, redondeando hacia arriba.
- Estadísticas de partido.
- Opciones de audio y visuales.
- Guardar nombres y ajustes.
- Menú principal limitado a «Jugar» y «Opciones».

### Duración

- Decisión confirmada el 31-10-2026: la duración máxima de un partido es de 5 minutos.
- Para la primera configuración se propone ofrecer 1, 2, 3 y 5 minutos (filtrando la lista inicial de 1, 2, 3, 5, 10 y 15 por el máximo confirmado). La lista definitiva de opciones menores o iguales a 5 no se ha confirmado expresamente.

## Prioridad de implementación

### v0.1 jugable

1. Inspeccionar y conservar los archivos existentes antes de modificar código.
2. Campo cenital completo y dos jugadores (azul controlado y rival bot).
3. Movimiento analógico 360° del jugador local; sin sprint.
4. Pelota con fricción, rebotes y colisiones razonables con jugadores y límites.
5. Gol, marcador legible con fuente real y saque de centro.
6. Disparo con A una vez por pulsación.
7. Temporizador con límite de configuración de 5 minutos, cuenta atrás y gol de oro si hay empate.
8. Compilar y entregar únicamente el .nro; verificar el archivo resultante y comunicar claramente qué se ha comprobado.

### Después de v0.1

Construir por etapas los menús, selección de bandos, mandos múltiples, equipos 1v1–4v4, dificultad de bots, estadios, pausa/desconexión/reconexión, revancha, estadísticas, opciones y guardado. Reflejar cada capacidad como implementada solo después de comprobarla en código/ejecución.

## Cuestiones aún abiertas

- Confirmar las duraciones ofrecidas por debajo del máximo de 5 minutos; propuesta actual: 1, 2, 3 y 5.
- Confirmar disponibilidad y coste de SDL2_ttf para Switch; si no resulta adecuado, emplear una fuente real prerenderizada de calidad. No volver a los segmentos ni a letras geométricas.
- Definir las reglas exactas de posesión para permitir pausa y la política de sustitución/reconexión.
- Especificar el comportamiento detallado de cada estadio/variante y las estadísticas.
- Confirmar si las partidas multijugador previstas son solo locales; el material recuperado no acuerda juego en línea.

## Disciplina de cambios y continuidad

- Leer este archivo, source/main.c, Makefile y README.md al comenzar trabajo nuevo.
- No tratar imágenes/mocks antiguos como guía visual.
- Antes de editar código, mantener copias seguras de los archivos que vayan a cambiar; no borrar artefactos anteriores.
- No inventar decisiones para cerrar huecos: registrar la propuesta y su estado como pendiente.
- Actualizar esta especificación después de cualquier decisión confirmada o cambio funcional verificado.
- Generar solo .nro; no incluir .elf como entrega solicitada.


## Implementación v0.1 verificada el 31-10-2026

- source/main.c ahora incluye campo cenital completo, jugador azul con stick analógico, rival bot rojo, colisión jugador-pelota y entre jugadores, fricción/rebotes, golpe A por pulsación, goles, reinicio y cuenta atrás 3-2-1.
- Incluye partido de 5 minutos, gol de oro en empate y estado de resultado final. La selección de duraciones menores todavía no está implementada.
- El marcador/texto usa un atlas prerenderizado y suavizado de Arial Bold (fuente real), embebido para no depender de SDL2_ttf en Switch. No se usa texto de siete segmentos ni letras pixeladas.
- La compilación de devkitPro/MSYS2 generó HaxBall-NX.nro correctamente. El archivo .elf es un intermediario del Makefile y no forma parte de la entrega solicitada. No se probó en una Nintendo Switch física.
- La salida compiló sin avisos después de corregir los avisos detectados en la primera compilación.
- Copias previas a los cambios: source/main.c.pre-v0.1-20261031.bak y Makefile.pre-v0.1-20261031.bak.
- Los archivos font_atlas.bmp y font_atlas.h son el activo y la versión embebible de la fuente prerenderizada.


### Ajuste por prueba de juego — 31-10-2026

- Si el balón queda casi inmóvil junto a un borde durante 2,5 segundos, vuelve al centro. El partido continúa inmediatamente, los jugadores conservan sus posiciones y el reloj sigue; no hay cuenta atrás.
- La velocidad máxima del balón queda limitada para evitar aceleración repetida por colisiones.

## HaxBall NX v0.2 — compilado el 31-10-2026

- Menú principal limitado a Jugar y Opciones. El orden acordado es: Menú principal → Selección de equipos → Todos listos (editar nombres/continuar) → Configuración de partido → Partido.
- Configuración de duración 1, 2, 3 o 5 minutos; equipos de 1v1 a 4v4, humanos por bando, dificultad del bot, estadio y nombres editables con el teclado de Switch.
- La selección de equipos configura tamaño, humanos azules/rojos y bots de relleno. Cada mando humano conectado confirma Listo individualmente en la pantalla Todos listos; ahí también se editan los nombres antes de continuar.
- Cuatro estadios con diferencias de superficie/color, fricción y tamaño de portería. Los bots cercanos al balón lo persiguen; los compañeros se colocan en apoyo siguiendo la posición del balón.
- Pausa manual al tener posesión; pausa automática por desconexión. La reconexión antes de 5 segundos reanuda; después, el jugador pasa a bot.
- Pantalla de resultado con marcador y estadísticas de disparos/toques; la revancha requiere votos de al menos la mitad de los mandos conectados, redondeando hacia arriba.
- Opciones de volumen (tono de gol) y contraste visual. Ajustes y nombres se guardan en la SD en switch/HaxBall-NX/settings.cfg.
- Si el balón se desplaza menos de 1,1 píxeles por cuadro junto a un borde durante 2,5 segundos, se coloca en el centro sin cambiar estado de partido, posiciones de jugadores ni reloj; la partida continúa sin cuenta atrás.
- Velocidad máxima del balón limitada para evitar aceleración por colisiones repetidas.
- El archivo HaxBall-NX.nro compiló con devkitPro/MSYS2 sin avisos. No se ha probado todavía en hardware.
- Pendiente: acabado visual de selección inspirado en FIFA y capitanes, variantes más detalladas por estadio, estadísticas ampliadas y conexión en línea. El modo local es el implementado.

### Correcciones tras la revisión del usuario — 31-10-2026

- Se corrigió la jerarquía de pantallas para que coincida con el flujo indicado por el usuario.
- Se cambió la detección de balón atascado para medir desplazamiento cuadro a cuadro junto a los bordes y evitar depender solo de la velocidad instantánea.
- La IA de apoyo ya no mantiene a los bots secundarios en posiciones fijas cuando un compañero humano va al balón: se mueven a posiciones de apoyo relativas al balón.
- Se recompiló HaxBall-NX.nro sin avisos. Falta validar el comportamiento en Switch física.

## Revisión funcional tras especificación completa — 31-10-2026

La especificación original se conserva en ESPECIFICACION_ORIGINAL_20261031.txt como referencia histórica. Su afirmación de que no existe prototipo está desactualizada. Su apartado 6.9 se sustituye por la decisión posterior del usuario: solo el balón vuelve al centro; el reloj, el partido y los jugadores continúan.

Cambios implementados y compilados, pendientes de prueba en consola:
- Selección de lado por mando independiente: rojo izquierda, centro no participa, azul derecha; A confirma y B cancela Listo antes de cambiar lado. Solo participantes bloquean el avance.
- El primer humano que confirma en cada lado es capitán. Todos listos permite A continuar, X editar nombre propio, Y editar equipo si es capitán; R permite al capitán editar el equipo contrario cuando solo tiene CPU. Los teclados se abren de uno en uno.
- Nombres personales y distribución se guardan en players.cfg. Los jugadores muestran su nombre durante el partido.
- Tamaño de equipos elegido en configuración; las plazas vacías se completan automáticamente. No se permite iniciar con más humanos que plazas. Duraciones 1,2,3,4,5.
- Pausa por Plus: equipo del último toque con jugador cercano; también disponible durante cuenta atrás para recuperación. Menú Reanudar/Reiniciar/Opciones/Salir a equipos. Reinicio pide A confirmar/B cancelar. Desconexión mantiene pausa; X sustituye desconectados solo si queda otro humano conectado.
- IA con objetivo revisado según dificultad: colocarse detrás del balón, atacar, pasar a compañero adelantado, apoyo e interceptación/protección temporal de portería. Reacción y error de tiro varían; velocidad 305 y fuerza 610 iguales a humanos. Es una heurística inicial, no inteligencia validada en consola.
- Atasco: si la pelota permanece dentro de 22 píxeles del punto de referencia durante 2,5 segundos, vuelve al centro sin pausa ni cuenta atrás. Se eliminó el filtro de borde y el criterio por cuadro. Los valores son provisionales para calibración.
- Contactos físicos responden a velocidad relativa de aproximación, evitando sumar fuerza repetidamente mientras se empuja una pared.
- Compilación devkitPro/MSYS2 sin avisos. No se han verificado estos cambios en hardware.

Pendientes del documento completo: diálogo nativo Conectar mandos, validación Joy-Con horizontal, dificultad independiente por equipo, música por estadio, geometrías especiales/restricción de balonmano, estadísticas ampliadas y acabado visual final. No considerar implementado todo el documento por estar incorporado al repositorio.

## Decisiones vigentes: controles y movilidad — 31-10-2026
Estas decisiones sustituyen las anteriores sobre contraste, volumen y tiro frontal.
- Quitar contraste. Opciones: volumen general, efectos de sonido, nombres sobre jugadores y vibración, todos activar/desactivar. Volumen general silencia todos los sonidos; efectos controla los tonos de tiro/gol. No hay música implementada. Guardado compatible con ajustes anteriores.
- Elegir lado: B cancela Listo; sin confirmar vuelve al centro; desde el centro vuelve al menú. Menos vuelve directamente al menú principal.
- A dispara una vez por pulsación en la dirección del movimiento analógico, incluyendo diagonales y hacia atrás. Quieto conserva última dirección (criterio de implementación); al saque empieza mirando a portería rival. Indicador visual de dirección.
- Bots: asignación por situación de disputa, cobertura, apoyo y desmarque; reasignación por distancias y rotación de apoyos. Separación entre compañeros, anticipación, evaluación de líneas de pase, conducción, disparo o despeje. Dificultad cambia tiempos y precisión, manteniendo velocidad/fuerza iguales a humanos.
- Se conserva recuperación del balón tras 2,5 s dentro de 22 px: solo balón al centro, jugadores/reloj siguen.
- Vibración breve al tiro humano y gol; apagado corta vibración activa. Compatibilidad física pendiente de prueba con cada mando.
- Tiro: ventana de 0,16 s sin colisión con el propio lanzador para que el tiro hacia atrás no rebote inmediatamente contra él; los demás contactos siguen activos. Lectura de nombres guardados corregida para longitud máxima de 15 caracteres.
- Verificación: compilación devkitPro correcta sin avisos. Pruebas de lógica extraída de C y adaptada a JavaScript: ocho direcciones de tiro, protección de contacto del lanzador, cuatro roles distintos en 4v4, rotación y reasignación al compañero cercano. No sustituyen prueba en Switch; audio, vibración, navegación con varios mandos y calidad del juego pendientes de validación física.

## Ajuste de sensacion y menus — 31-10-2026
Peticion: mas control de balon, golpeo con sensacion de potencia y renovar interfaz. No se afirma replica exacta de HaxBall.
- Interpretacion provisional a probar: aceleracion/frenada cortas, stick con respuesta proporcional tras zona muerta, contacto suave para conducir y rebote en recepciones fuertes. No se pega automaticamente el balon al jugador.
- A sigue siendo un golpe por pulsacion hacia la direccion elegida, sin carga. Sustituye fuerza fija anterior: impulso base 740, modificado por avance del jugador y movimiento previo del balon, limitado a 690–880; mismo calculo para humanos y bots. Limite de velocidad 900. Ajustes sujetos a prueba del usuario.
- Menus renovados: fondo azul oscuro, acento menta y equipos rojo/azul, paneles espaciados y tipografia real proporcional a partir del atlas existente. Se mantiene el flujo y controles acordados.
- Implementado contacto suave (restitucion 0,08 bajo velocidad relativa 330; 0,50 para impactos fuertes), aceleracion exponencial 22/s y frenada 30/s. Balon integrado en pasos de como maximo 1/120 s para reducir saltos en impactos potentes. Recuperacion de atasco sigue afectando solo al balon.
- Verificacion: compilacion sin avisos. Pruebas de funciones C extraidas/adaptadas a JS: ocho direcciones, potencia al avanzar, limite de fuerza, conduccion suave, recepcion fuerte y aceleracion/frenada a 30/60/120 Hz. Revision visual local reconstruida desde funciones de dibujo y atlas para menu principal y opciones; no es captura de Switch. Falta validacion fisica de sensacion de juego y menus con mandos.

## Joy-Con horizontal, espectadores y tiro al soltar — 31-10-2026
- Confirmado por el usuario: disparar al soltar. Sustituye tiro en el flanco inicial de A. Toque corto dispara al soltar sin retraso adicional. Mantener abre hasta 1 s de orientacion, despues fija la direccion hasta soltar, sin tiro automatico ni acumulacion de potencia. Durante apuntado se frena el movimiento; criterio provisional para poder orientar, pendiente de prueba del usuario.
- Indicador de pulsacion y apuntado visible con o sin balon. El lanzamiento solo sucede si el balon esta al alcance al soltar. Pausa, desconexion y saque cancelan tiros pendientes.
- Navegacion con cruceta o stick, umbral y repeticion temporizada por mando; disparos no se repiten al mantener. Joy-Con izquierdo/derecho horizontal: botones por posicion derecha=A, inferior=B, superior=X, izquierda=Y; +/- pausa, SL volver al menu en elegir lado y SR editar equipo CPU. Stick rotado segun lado. Se mantienen Pro, pareja y portatil.
- Y en Elegir lado abre conexion nativa; SL+SR permite registrar Joy-Con sueltos. Despues de cambiar conexion todos vuelven al centro y confirman de nuevo, evitando asignaciones antiguas a otro mando.
- Mandos en centro no requieren Listo, no bloquean avance ni votan revancha ni controlan la configuracion de participantes. Hace falta al menos un humano con equipo. B permite quitarse de un equipo (primero cancelar Listo).
- Jugadores circulares con contorno, sombra, volumen, dorsal, indicador de direccion y aro de apuntado; fuente real. El interruptor de nombres incluye CPU.
- Verificacion final: compilacion sin avisos. Pruebas de logica C extraida/adaptada a JS: botones y ejes Joy-Con horizontal, repeticion de navegacion, un participante con siete mandos neutrales, tiro solo al soltar tras mas de un segundo, indicacion de intento sin balon y cancelacion por pausa. Pendiente prueba fisica de Joy-Con izquierdo/derecho, applet nativo y controles multijugador. No afirmar compatibilidad fisica validada.

## Correccion del control de balon y aspecto — 31-10-2026
Sustituye expresamente la interpretacion anterior de detenerse y orientar un tiro durante un segundo.
- Mantener A NO inmoviliza. Durante el primer segundo desde la pulsacion, con balon cercano y no demasiado rapido, ayuda a mantenerlo frente al jugador mientras se mueve y cambia de direccion. Es control temporal, no carga de potencia. Al superar el segundo la pelota queda libre sin disparo automatico; el tiro solo sale al soltar si esta al alcance.
- Se puede cambiar direccion de movimiento siempre. La ayuda tiene alcance limitado y se pierde ante un rival cercano. Solo un jugador controla el balon; un disparo, saque, pausa o recuperacion de atasco libera el control.
- Indicacion de boton: solo aro uniforme, incluso sin balon; sin flecha, punto de orientacion ni barra/circulo de progreso. Jugadores planos azul/rojo como los de la ilustracion del menu, sin dorsal, brillo ni contorno blanco. Nombres conservan su interruptor.
- Joy-Con siempre normalizados a horizontal. Se solicita modo individual al iniciar y se separan automaticamente parejas detectadas, tambien al reconectar. Si cambia durante un partido se pausa para recuperacion. Pro y portatil mantienen sus controles. El dialogo Y queda como alternativa para emparejar, no como requisito para elegir horizontal.
- Verificacion: compilacion sin avisos y pruebas de funciones C adaptadas a JS para control en movimiento, giro, liberacion al segundo sin disparo, tiro al soltar y disputa rival. Revisado que no se dibuja orientacion/progreso ni se anula movimiento. Pendiente prueba fisica del tacto de balon y separacion/reconexion de Joy-Con.

## Mando integrado, toque corto y textos — 31-10-2026
- Mando integrado separado del primer externo: 8 fuentes externas mas Handheld independiente, sin mezclar botones/sticks. Hasta 8 jugadores en partido, 4 por equipo; fuentes sobrantes quedan en centro. Handheld solo aparece si el sistema lo comunica conectado, no se simula un mando inexistente.
- Se consulta modelo de consola para identificar Switch Lite (Hoag), con alternativa Mando integrado si no hay acceso al dato. Se muestran tipos Joy-Con izquierdo/derecho, pareja y Pro / compatible. Los mandos de terceros que se anuncian como FullKey no pueden distinguirse con certeza de Pro; no se inventa marca/modelo.
- Toque corto A: tiro al soltar, sin recoger previamente. Umbral de mantener 0,12 s; tras superarlo, ayuda de control hasta 0,5 s, sin inmovilizar ni cargar potencia. No puede decidirse al instante si una pulsacion futura sera corta o mantenida: el toque dispara en la liberacion.
- Textos sin esloganes: Partido local, numero de jugadores, equipos, ajustes y controles. Tipo de mando visible junto al jugador en Elegir lado.
- Verificacion: compilacion sin avisos; pruebas de logica C adaptadas a JS para toque sin captura, mantener/control en movimiento, expiracion, tiro al soltar y disputa. Revisadas mascaras independientes de mando integrado/externos. Falta prueba fisica con Switch Lite y combinaciones de mandos.

## Iconos Joy-Con y colores — 31-10-2026
- Referencia del usuario: rombo de cuatro botones con posicion activa resaltada, junto al boton del mando normal. Dibujado en codigo a resolucion de pantalla: A derecha, B abajo, X arriba, Y izquierda. Guia contextual en menus, pausa y partido.
- Jugador rojo con contorno blanco; azul con contorno oscuro. Colores mas saturados, compartidos con ilustracion de menu: rojo RGB 244/60/80 y azul 46/139/255. Se conserva aro de pulsacion y ausencia de flecha/punto/barra.
- Verificacion: compilacion sin avisos; revision de reproduccion local del dibujo con cuatro equivalencias visibles sin solapamiento (no captura de consola). Pendiente comprobacion en Switch.

## Limpieza de guias y paleta unica — 31-10-2026
- A/B/X/Y sin circulo. Rombo Joy-Con ampliado, cuatro botones solidos separados, activo azul y resto blanco; primitivas a resolucion nativa sin textura reescalada. Se eliminan bordes pequenos superpuestos.
- Una sola indicacion por accion en la guia inferior. Se retiran repeticiones en menu, opciones, equipos, nombres, configuracion, pausa y resultados. Indicaciones adicionales solo para controles distintos (stick, R/SR, pausa/salir).
- Paleta centralizada para TODAS las superficies de equipo: azul RGB 0/102/255 (#0066FF), rojo RGB 244/40/64 (#F42840). Jugadores, ilustracion principal, bandas de equipos y porterias usan esas constantes. Contorno rojo blanco; contorno azul gris neutro. El azul del icono Joy-Con es el mismo azul saturado. Se corrigen porterias para corresponder a lado inicial rojo izquierda / azul derecha.
- Compilacion sin avisos. Revision visual local reconstruida desde funciones de dibujo: iconos mayores y separados, letras sin circulo y Elegir una sola vez. Pendiente comprobar nitidez en pantalla de Switch.

## Iconos Pokedoku, tipografia y HUD — 31-10-2026
- Iconos de mando originales extraidos de los cuatro PNG incrustados en PokeDoku-NX/source/main.cpp (left/right/pro/handheld). Se conserva su mascara alfa y se colorea con paleta de equipo o blanco en centro. Joy-Con rotados horizontalmente como en Pokedoku. Sin dependencia nueva de SDL_image; recursos incrustados en controller_icons.h.
- Fichas de equipos mayores, nombre del jugador legible e icono en lugar del texto de tipo de mando. Centro con mas de 4 mandos usa dos columnas para hasta 9 fuentes. El jugador conserva su nombre, no se sustituye por un icono.
- Cuatro botones de ayuda un poco menores (radio 7, separacion 13); A/B/X/Y siguen sin circulo. Menu principal permite salir con Plus o Minus; Joy-Con normalizado admite ambos.
- Se ocultan nombres CPU, frase de capitan/equipo CPU y ayuda Y Equipo inferior, conservando controles de edicion existentes. Se retiran instrucciones de toque/mantener/pausa y leyenda de accion durante juego y pausa ordinaria; se mantienen confirmacion de reinicio y recuperacion por desconexion.
- Fuente real Segoe UI Semibold prerenderizada, reemplazando atlas Arial anterior; sin letras geometricas ni pixeladas. Recursos generados desde la fuente instalada en Windows.
- Marcador en franja superior: rojo izquierda, azul derecha, resultado central y reloj inferior. Cuenta atras en disco oscuro central con anillo de tiempo y digito grande, dibujada sobre el juego.
- Verificacion: compilacion sin avisos; cuatro mascaras originales verificadas de 100x100; revision visual reconstruida del menu con fuente nueva, botones reducidos y salida +/-. Nombres largos reducen avance para no salirse de la ficha. Pendiente revision completa en consola de iconos, HUD y cuenta atras.

## Pareja Joy-Con y revision visual — 31-10-2026
- Se revoca separacion automatica obligatoria: se respetan parejas JoyDual elegidas en el sistema, tanto al arrancar como al reconectar. Y abre conexion nativa con parejas permitidas; L+R registra pareja y SL+SR Joy-Con individual. Solo los individuales usan rotacion/mapeo horizontal; JoyDual usa botones/stick normales. Icono de pareja usa los dos originales de Pokedoku juntos.
- Todos listos: columna roja izquierda y azul derecha, cada una vertical hasta 4 humanos; equipo, mando, nombre y capitan visibles.
- Tipografia Bahnschrift real prerenderizada a 29 px, sustituyendo Segoe UI Semibold 26; etiquetas de filas y subtitulos mayores. Icono de cuatro botones conservado.
- Opciones y ajustes de partido muestran < a la izquierda y > a la derecha en la fila ajustable seleccionada. Filas de accion solo conservan >.
- Aro de pulsacion blanco; jugadores sin borde negro/blanco; balon completamente blanco sin punto ni sombra. Porteria neutra con red y marco blanco, sin relleno de equipo. Geometria de gol no cambia.
- Salir +/- con signos blancos dibujados a grosor fijo para legibilidad, sin depender de un guion fino de la fuente.
- Compilacion sin avisos; revision local reconstruida de fuente y signos +/-; comprobado que ya no existen llamadas de separacion forzada. Pareja/sueltos y aspecto final pendientes de prueba en consola.

## Correcciones de conexion, interfaz, audio y goles — 01-11-2026
- El usuario identifica aviso nativo de Switch «modo con dos mandos no compatible». Se cambia apertura a ControllerSupportForSystem con hold vertical durante el dialogo y retorno horizontal solo para singles; fallback a ControllerSupport. Se añade emparejamiento L+R directo de Joy-Con izquierdo/derecho en Elegir lado para no depender del applet. Pendiente reproducir y verificar en hardware; no dar el fallo por resuelto solo por compilar.
- Salir +/- pasa a la misma linea inferior y tamaño de las ayudas A/B/X/Y. HaxBall turquesa. Valores Activado/Desactivado centrados; flechas dentro del valor en opciones/configuracion. Cuenta atras con circulo completo y digito centrado por limites reales del glifo. Resultado baja 5 px y reloj sube 4 px.
- Atlas ampliado a caracteres latinos con decodificacion UTF-8: tildes y ñ reales. Corregidos textos Duración, Vibración, Música, Fútbol, Tamaño, Atrás, Capitán, Clásico, Fácil, Difícil y otros.
- Porterias mas altas (clasico semialtura 96; sala 76; hockey 68; balonmano 84) y profundidad 32. Postes fisicos radio4, balon rebota por normal. Impacto con velocidad de aproximacion >45 produce vibracion y tono. Gol normal y gol de oro vibran con opcion habilitada.
- Musica original sintetizada en bucle, mezclada con efectos en cola SDL. Opcion independiente Musica de fondo, guardada como cuarto valor al final; compatible con archivos anteriores. Volumen general silencia todo y efectos controla solo golpes/goles.
- Se reafirma soporte NpadStandard (incluye JoyDual) despues de inicializar SDL y antes de abrir applet, evitando que la inicializacion de otro subsistema deje una configuracion distinta. No se afirma que esa sea la causa demostrada del aviso.
- Verificacion: compilacion sin avisos, pruebas de logica C adaptada a JS para rebote/poste sin pulsos repetidos, musica/efectos/master y muestras finitas sin clipping. Revision visual reconstruida de menu/opciones con fuente latina. Pendiente Switch: applet de mandos, pairing directo, audio y vibracion fisicos.

## Campos adaptativos y hockey arcade — 01-11-2026
Las cuatro referencias aportadas en este turno pasan a ser referencias vigentes para colores y marcas de los campos; se conserva vista cenital, no perspectiva isometrica.
- Correccion explicita del usuario: TODOS los modos admiten 1–4 por equipo. No limitar futbol/sala. Dimensiones, radio de jugadores/balon y apertura de porterias dependen de cfg.size, incluidos bots. Mas participantes: campo mayor y cuerpos menores; sala mantiene jugadores algo mayores y porterias menores que futbol.
- Futbol: cesped con franjas, circulo central, areas y medias lunas. Sala: superficie azul, circulo central, areas semicirculares y esquinas. Balonmano: superficie amarilla, areas azules y arcos continuos/discontinuos; dimensiones segun participantes. Balonmano conserva reglas arcade de contacto/gol (no se han solicitado reglas reglamentarias adicionales).
- Hockey arcade confirmado: disco, bandas, porterias 100 px adelantadas y espacio por detras, postes/redes solidos para disco y jugadores. Solo cuenta gol si el disco cruza por delante hacia el interior, no al tocar la parte trasera. Sin icing, fuera de juego ni sanciones.
- Publico representado por filas de circulos en los margenes de cada pista. No son obstaculos fisicos.
- Se aumenta retencion de velocidad: futbol .993, sala .990, hockey .996, balonmano .992 por paso de 1/60 s. Es calibracion inicial pendiente de prueba.
- Texto normal +10%, incluidos nombres de acciones; A/B/X/Y y rombo Joy-Con sin cambio. Se restauran ayuda de capitan, R/SR equipo contrario y Y/equipo inferior.
- Verificacion final: compilacion sin avisos; pruebas de funciones C adaptadas a JS de las 16 combinaciones arena/tamano, radios y porterias monotonicamente adaptativos, goles hockey solo por frente, rebote red posterior, espacio tras porteria, bandas y goles de otros modos. Revision visual reconstruida de los cuatro campos en 3v3. Falta hardware para tacto, legibilidad completa y rendimiento. Tras separar colisiones entre jugadores se limita posicion al campo y se resuelven redes de hockey.

## Presentacion de partido, estadisticas y rendimiento — 01-11-2026
- Estadisticas separadas rojo/azul: goles en cabecera, golpeos (tiros/pases), contactos con intervalo minimo 0,15 s por jugador, postes, goles en propia puerta y posesion cercana estimada (ultimo equipo en tocar y jugador de ese equipo a menos de 110 px). No se presenta como posesion reglamentaria exacta.
- Fichas: nombres y estado centrados en el espacio junto al icono, titulo principal y subtitulos Jugar/Opciones mayores.
- Velocidad maxima objetivo de jugadores baja de 305 a 245; golpe base 570, intervalo520–680 y limite balon710, iguales para bots y humanos. Radios de 1v1 y 2v2 bajan, con reduccion mas gradual al subir equipos.
- Exterior con pasillo de seguridad, barrera, tres filas escalonadas de gradas, figuras circulares con camiseta/cabeza, sectores de equipo y banderas animadas. Campo ligeramente reducido en pantalla para dejar espacio exterior.
- Area de futbol basada en nueva referencia: area grande completa, area pequena y arco corto exterior. Marcador con dos bloques de resultado de color y reloj central.
- Fondo desenfocado y oscurecido dibujado antes de abrir teclado nativo; captura/reduccion solo al abrirlo. Presentacion final del teclado depende del applet de Switch.
- Cuenta atras con aro turquesa por segundo y digitos reales prerenderizados a mayor resolucion y dibujados sin ampliar. Animacion de gol de 1,4 s con equipo, particulas y vibracion mas larga; reloj detenido durante celebracion, despues saque o resultado (incluye gol de oro).
- Rendimiento: estadio estatico cacheado en una textura por modo/tamano; animacion exterior ligera aparte. Bucle musical precalculado con las mismas notas, sin pow/exp/sin de musica durante el partido. Se intenta renderizador acelerado con vsync, con alternativa software. No afirmar eliminacion de lag sin medir en consola.

- Verificacion: compilacion sin avisos; pruebas de estadisticas por equipo, intervalo de contactos, gol en propia puerta, celebracion sin avance de reloj, saque y resultado tras gol de oro. Las 16 combinaciones de estadio/tamano mantienen dimensiones y colisiones verificadas. Revision local reconstruida del campo de futbol y sus gradas. Pendiente en Switch: fluidez real, teclado desenfocado, vibracion y legibilidad en pantalla.

## Ajuste deportivo — 01-11-2026
Duraciones disponibles 1/2/3 minutos; ajustes antiguos de 4/5 pasan a 3. Balonmano: pista amarilla con mosaico, areas azules de extremos redondeados; nadie de campo entra en ninguna area. Se añade un portero automatico adicional por equipo, pequeno y lento, sin quitar plazas humanas. Hockey: esquinas redondeadas fisicas, redes con marco rojo y aro de tiro negro. Jugadores mas lentos, tiro de primeras mas rapido que suelta tras control efectivo. Marcador centrado por tinta real de fuente. Etiquetas Tiros y Posesion (se conserva medicion estimada existente).

Ampliacion del mismo turno: eliminar vibracion y su opcion (compatibilidad de lectura del antiguo ajuste); musica original distinta menu/partido y motivos de gol/victoria/derrota, efectos renovados. B vuelve de pausa si no hay mandos desconectados. Todos listos centrado; fuente a doble resolucion con transparencia real. Campo ampliado, marcador compacto superpuesto a gradas, reloj rojo ultimos15s. Aficiones reaccionan al resultado de su equipo. Cuenta atras inicial3s, tras gol1s. Texto 1 vs 1 hasta 4 vs 4 y L+R / SL+SR emparejar.

Implementacion y verificacion final: campo ampliado (1000–1136 px de ancho segun deporte/equipos), velocidad jugadores205, tiro inmediato430–530 frente a suelta controlada330–365, limite balon550; mayor friccion para priorizar colocacion. Porteros automaticos radio11 y velocidad65. Se conserva medicion aproximada de posesion y solo cambia etiqueta. Fuente Bahnschrift a doble resolucion y alpha real, marcador centrado por limites de tinta. Musica de resultado toma como perspectiva el primer humano conectado con equipo; aficiones de ambos equipos reaccionan visualmente por separado. Motivos y bucles originales precalculados, sin coste de sintesis musical en partido. Sin llamadas de vibracion y sin opcion visible; campo legacy de ajuste se lee y se guarda a0. Compilacion sin avisos. Pruebas de C adaptado a JS:16 combinaciones, goles y redes hockey, esquinas, exclusion areas, portero, tiro controlado/directo, estadisticas y secuencia gol. Revision local reconstruida de campos. Pendiente prueba real Switch de sonido, legibilidad y rendimiento.

## Circulos, control breve y cuatro porterias — 01-11-2026
Peticion vigente: circulos redondos sin picos en botones y cuerpos, aro de tiro negro en todos los deportes, control retenido0,25s, aficion mayoritaria reactiva. Marcas sala6/10m como puntos; balonmano4/7m como lineas. Porteros de balonmano se desplazan en2D dentro del area. Nuevo estadio arcade cuatro porterias entre esquinas impulsoras; cada portero se mueve solo por su linea.1 bola al inicio,2 al75% de tiempo restante,3 al50%,4 al25%. Puntuacion y reparto humano pendientes de respuesta; no fijarlos como decision verificada hasta resolver. Revisar coste del dibujo de circulos/anillos y transiciones.

Cuatro porterias confirmado: hasta4 humanos, bots autocompletan. Puntuacion combinada de goles marcados y encajados por tiempo: implementacion+1 marcado/-1 encajado, atribucion al ultimo portero que toque; autogol solo resta. Empates comparten resultado. Se asignan jugadores participantes por orden de mando a rojo(arriba),verde(derecha),azul(abajo),amarillo(izquierda); si hay mas de4 participantes se exige volver a elegir lado y dejar sobrantes sin equipo. Reaparicion individual de bola tras gol sin detener las demas. No se aplica agarre del balon a este modo; A impulsa el rebote cercano.
Correccion: cuatro porterias ignora tamano de equipos y exige solo maximo4 humanos. Sala dos marcas por extremo al12/20% del largo; balonmano marcas cortas4/7m al10/18%, dentro/fuera del area. Arena arcade con bandas luminosas.
Revision gameplay: cuatro circulos con radio independiente del color, atribucion individual de goles y salida aleatoria. Balonmano coral/azul, portero busca pase despejado o detiene balon. Aros blancos salvo hockey negro. Anuncio de equipo y goleador/CPU.
Revision: cache del escenario cuatro porterias (antes se redibujaba completo por fotograma), reserva audio por lotes. Inicio en posiciones de porteria y cuenta atras visible. Marcadores laterales, pilares salientes, tiro al soltar y aro blanco, colores elegidos con alternativas para duplicados. Balonmano tono ocre y reintento de pase seguro.
Portero: pase con impulso430 y balon fuera de radio de contacto para evitar recaptura inmediata. Arena: esquinas78, salida central, sin marcas centrales ni segmentos de gol, luces cuatro lados en tonos de jugador, pausa ampliada.
Bots arcade: disparo automatico cercano con cooldown0.65/0.4/0.22s segun dificultad y aro blanco.
Modo cuatro porterias: incorporadas colisiones elasticas entre bolas, ligeras variaciones de angulo en tiros de bots para evitar trayectorias repetitivas, reloj en panel propio.
Cuatro porterias: aviso de gol solo nombre/CPU y GOL en color del autor, con autor guardado por gol independientemente de otras bolas. Avisos y tiempo fuera del campo en columnas laterales. Pelota radio15 con fisica equivalente, independiente del tamano de equipo.
Cuatro porterias: aviso de gol debajo de la ficha del autor, no del receptor. Resultados en tabla vertical de cuatro jugadores, cabeceras unicas Jugador/GF/GC/Puntos, nombre humano o CPU con color de participante.
Cuatro porterias: propulsores con rebote elastico y multiplicador1.45, velocidad minima de salida460 y limite650. Sorteo de lados al iniciar/reiniciar/revancha: se mueve junto jugador, mando, color y nombre, sin repetir lado del mismo participante respecto a partida anterior durante esta ejecucion.
Revision IA: poseedor avanza aunque su rol fuera apoyo/cobertura; evalua varios puntos de tiro y pases con progreso, umbrales por dificultad. Arena centrada con margen para pilares; votos visibles. Revision local sin perfilado Switch: no afirmar eliminacion definitiva de tirones.
Audio de usuario integrado como PCM mono22.05k: click para navegacion y poste; chute al impulso; inicio en salida; gol sobre musica; win para victoria, goal fallback derrota; music menu/partido independientes. Assets convertidos sin decodificacion en juego.

## Ajuste de mezcla y cuenta atras — 01-11-2026
Musica del partido elevada frente al nivel anterior; chute atenuado para igualar volumen percibido del golpe al poste. Victoria reproduce win.wav al terminar cualquier partido, tambien con ganador CPU. pitido.wav se segmenta en tres pitidos cortos para 3/2/1; se descarta el tono largo. Los sonidos de cuenta atras se disparan al cambiar cada numero, no desde el saque.

Aclaracion de audio por marcas del usuario: segmentos de pitido tomados de izquierda a derecha para 3/2/1, excluyendo el largo. En partido, mezcla objetivo de SFX igualada por RMS y ligeramente inferior al chute anterior; musica de partido solo un poco por encima de la de menu.

Balance de audio confirmado: menu, partido y win.wav normalizados al mismo RMS. Grupo SFX (click/poste/chute/gol/3/2) normalizado unos 10 dB por debajo; pitido 1 queda 0.5 dB por encima. Inicio partido/ronda no se reproduce.

Ajuste de nivel solicitado: pitido1 igualado RMS a pitidos2/3; ganancia global de salida x1.20 para todos los sonidos. Musica de menu atenuada para quedar aproximadamente7% por debajo de su nivel anterior tras el aumento global; resto de pistas y efectos sube20%.

Correccion: incremento uniforme x1.20 aplicado tambien a las dos musicas de fondo; no hay atenuacion de menu despues del aumento.

Aclaracion final de niveles: pista menu atenuada10% antes del aumento general x1.20 aplicado al audio completo; en salida queda aprox8% por encima de nivel anterior y algo por debajo del partido.

Aumento adicional de audio: +30% sobre la compilacion anterior, acumulado al x1.20 previo (factor global x1.56 sobre base). Se conservan niveles relativos/menu-partido.

Cuatro porterias: marcadores individuales situados junto a su participante segun lado (arriba/abajo/izquierda/derecha); avisos de gol se acercan al goleador. Regla de propia: gol en la propia porteria resta1 al portero/autor y suma GC sin GF.

Cuatro porterias: gol sin ultimo toque (por ejemplo, saque directo) resta al receptor y el aviso solo muestra GOL, sin adjudicarlo a CPU ni a participante.

Cuatro porterias: marcador en laterales, asociado al puesto del jugador; superior e izquierdo usan columna izquierda a distinta altura, derecho e inferior columna derecha.

Cuatro porterias: goles en propia (ultimo toque del dueño de esa porteria) y goles sin ultimo toque solo restan1 al receptor; no muestran aviso ni sonido de gol. Goles atribuidos a otro participante conservan puntos, aviso y sonido.

Correccion cuatro porterias: columnas laterales restablecidas a alturas235/465 y asignadas por cercania (superior/derecho arriba, inferior/izquierdo abajo). Porterias de los cuatro deportes aumentan profundidad visual/fisica neta de32 a40px; cuatro porterias queda igual.

Goles deportes normales: registrar al cruzar la linea de gol; animacion de celebracion mantiene pelota en movimiento por red hasta tocar fondo, luego detiene. Avisos cuatro porterias situados justo debajo de sus tarjetas laterales en alturas235/465.

Animacion de gol: la pelota ya no se congela al tocar el fondo; rebota con amortiguacion dentro del bolsillo de la red hasta acabar la celebracion.

Ajuste naturalidad de gol: profundidad neta48px; se elimina impulso minimo y snap de entrada, preservando velocidad exacta al cruzar. Celebracion se alarga de1.4 a1.9s antes del saque.

Correccion de red: el balon no queda enganchado ni se congela; sigue moviendose de forma continua dentro de la red con rebotes suaves en fondo y laterales, y amortiguacion progresiva. Tiros fuertes pueden salir de nuevo con rebote reducido. Velocidad de entrada sin impulso artificial.


Hockey — marcas de pista: adaptar el campo a la referencia cenital: rectangulo con esquinas redondeadas, lineas azules de zona a 1/3 y 2/3, linea central roja, circulo y punto central azules, cuatro circulos de saque rojos con sus marcas y lineas de gol rojas adelantadas a ~14% de cada extremo. Porteria con areas semicirculares azules.

Revision referencia hockey: proporciones de circulos/lineas tomadas de la imagen, areas azules rellenas y porterias adelantadas al16%. Red: movimiento libre y friccion normal antes del contacto; amortiguacion solo al tocar fondo/lateral, sin retener en la boca. Video93.754s inspeccionado por secuencia a2fps; pendiente valoracion en consola.

Hockey referencia exacta: hielo blanco, borde azul, cuatro circulos rojos con cruces y puntos, puntos neutrales rojos, centro azul y linea central roja, areas rellenas cian. Lineas de gol al7.3% y porterias ligeramente adelantadas al9%.

Rendimiento: mezcla de audio limitada a2 bloques por fotograma, ganancias fuera del bucle de muestras y cursor circular sin modulo por muestra. Registro en memoria de fotogramas>25ms (tiempo total/audio/pantalla/estadio); escritura performance.csv solo al salir. Requiere prueba real para confirmar origen de tirones.

Hockey: linea roja y area semicircular alineadas con boca de porteria; porterias al13% del largo para ampliar espacio detras de las redes.

Hockey: conservar espacio posterior; circulos de saque centrados entre linea de gol y linea azul para evitar cruces. Area azul radio igual a semiancho de boca+4px, alineada a la boca y linea de gol.

Version1.0.0, autor Terremotix. Solo multijugador local, sin online. Icono del usuario integrado en NRO/NACP y pantalla de presentacion incrustada en RomFS, visible al inicio y omisible con boton. Precarga de circulos/anillos usados durante juego y reloj de alta resolucion para evitar cuantizacion de dt. No se garantiza ausencia de tirones sin medicion real; conservar performance.csv.

Mezcla: aumento adicional uniforme del20% aplicado al volumen maestro (musicas, eventos y efectos).
Cuatro porterías: marcador y reloj ampliados y menor fricción. Todos los modos bloquean volver y votar durante los cinco segundos de resultados. Traducción de valores, nombres predeterminados y resultados antes de componer las etiquetas.
