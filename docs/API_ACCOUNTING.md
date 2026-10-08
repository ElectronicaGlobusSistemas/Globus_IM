# Módulo API_Accounting

## Resumen Ejecutivo

El **Módulo API_Accounting** es responsable de la transmisión de contadores acumulativos de la máquina de juego hacia el servidor backend Gmaster. 

**Característica clave de diseño**: Los contadores son valores acumulativos (no instantáneos), por lo que el sistema es **tolerante a pérdidas parciales de transmisión**. Si una transmisión falla, la próxima transmisión exitosa enviará el valor correcto acumulado.

## Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                    API_Accounting Module                     │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Queue-based Processing                   │   │
│  │  • QueueAccounting (FreeRTOS Queue)                   │   │
│  │  • AccountingRequest_t (Request Structure)            │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              HTTP Communication                       │   │
│  │  • POST /api/Tarjeta/ProcesarContadoresApi (Port 9090) │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## Estructuras de Datos

### AccountingRequest_t

```cpp
typedef struct {
    int Com;           // Comando/identificador de la solicitud
    bool IsSuccess;    // Estado de éxito de la operación
    String Message;    // Mensaje descriptivo
} AccountingRequest_t;
```

### Contadores Administrados

| Constante | Descripción |
|-----------|-------------|
| `Total_Cancel_Credit` | Total de cancelaciones de crédito |
| `Coin_In` | Monedas ingresadas |
| `Coin_Out` | Monedas retiradas |
| `Jackpot` | Premios jackpot |
| `Total_Drop` | Total de drop |
| `Cancel_Credit_Hand_Pay` | Cancelación de crédito por handpay |
| `Bill_Amount` | Monto de billetes |
| `Physical_Coin_In/Out` | Monedas físicas entrada/salida |
| `Total_Coin_Drop` | Total de drop de monedas |
| `Ticket_In/Out` | Tickets entrada/salida |
| `Games_Played` | Partidas jugadas |
| `Current_Credits` | Créditos actuales |
| `Machine_Paid_Progresive_Payout` | Pagos progresivos máquina |
| `Machine_Paid_External_Bonus_Payout` | Pagos bonus externos máquina |
| `Attendant_Paid_Progresive_Payout` | Pagos progresivos operador |
| `Attendant_Paid_External_Bonus_Payout` | Pagos bonus externos operador |
| `Door_Open` | Contador de aperturas de puerta |
| `Games_Since_Last_Power_Up` | Partidas desde último encendido |

## Métodos Públicos

### Inicialización

#### `Init_Cola_Accounting()`
```cpp
bool Init_Cola_Accounting(void);
```
- **Propósito**: Inicializa la cola de FreeRTOS para procesamiento de accounting
- **Capacidad**: 3 elementos
- **Retorna**: `true` si se creó exitosamente, `false` en caso contrario

### Transmisión de Contadores

#### `Transmite_Contadores_Api_Cola()`
```cpp
bool Transmite_Contadores_Api_Cola(int Comando, bool IsOk, String Messages);
```
- **Propósito**: Envía una solicitud de actualización de contadores a la cola
- **Parámetros**:
  - `Comando`: Identificador del comando (3 es el estándar usado)
  - `IsOk`: Estado de éxito de la operación
  - `Messages`: Mensaje descriptivo
- **Nota**: Actualmente no asigna valores al request antes de enviar (BUG POTENCIAL)

#### `Actualiza_Modulo_Accounting()`
```cpp
bool Actualiza_Modulo_Accounting(int Com, Contadores_SAS contadores, 
                                 ESP32Time RTC, Variables_Globales Variables_globales,
                                 bool IsSuccess, String Message);
```
- **Propósito**: Transmite contadores completos al servidor backend
- **Endpoint**: `POST http://[IP_Server]:9090/api/Tarjeta/ProcesarContadoresApi`
- **Headers**:
  - `Content-Type: application/json`
  - `hash`: [Hash de validación]
  - `gmsec: GMaster`
  - `Authorization: Bearer [Token]`
- **Timeout**: 10 segundos

**Payload JSON**:
```json
{
  "IsSuccess": true,
  "Message": "Solicitud actualizar Modulo Accounting",
  "Comando": 3,
  "Maquina": {
    "Id_Machine": "12345",
    "Ip": "192.168.1.100",
    "Mac": "AA:BB:CC:DD:EE:FF",
    "ROM_Signature": "ABCD",
    "Serie_Trama": 12345,
    "Maquina_En_Juego": 1,
    "Evento_Maquina_En_Juego": 0
  },
  "Contadores": {
    "Total_Cancel_Credit": 0,
    "Coin_In": 0,
    "Coin_Out": 0,
    "Total_Drop": 0,
    "Jackpot": 0,
    "Physical_Coin_In": 0,
    "Physical_Coin_Out": 0,
    "Total_Coin_Drop": 0,
    "Machine_Paid_Progresive_Payout": 0,
    "Machine_Paid_External_Bonus_Payout": 0,
    "Attendant_Paid_Progresive_Payout": 0,
    "Attendant_Paid_External_Bonus_Payout": 0,
    "Ticket_In": 0,
    "Ticket_Out": 0,
    "Cancel_Credit_Handpay": 0,
    "Bill_Amount": 0,
    "Games_Since_Last_Power_Up": 0,
    "Games_Played": 0,
    "Open_Door": 0,
    "Current_Credit": 0
  },
  "Jugador": {
    "ID_Jugador": 12345678,
    "Id_Operador": 87654321
  },
  "Fecha_Hora": "2026-08-21 14:30:00"
}
```

### Procesamiento de Tareas

#### `Task_Accounting()`
```cpp
void Task_Accounting();
```
- **Propósito**: Tarea que procesa solicitudes de la cola
- **Comportamiento**:
  - Lee requests de `QueueAccounting` con timeout 0 (no bloqueante)
  - Llama a `Actualiza_Modulo_Accounting()` con valores fijos
  - **Nota**: El timeout 0 significa que solo procesa si hay datos disponibles

## Flujo de Datos

```
┌──────────────┐
│ Contadores   │
│   SAS        │
└──────┬───────┘
       │
       ▼
┌───────────────────────────┐
│ Transmite_Contadores_Api  │
│         _Cola()            │
└──────┬────────────────────┘
       │
       ▼
┌───────────────────────────┐
│   QueueAccounting          │
│   (FreeRTOS Queue)         │
│   Capacidad: 3 elementos   │
└──────┬────────────────────┘
       │
       ▼
┌───────────────────────────┐
│    Task_Accounting()      │
│   (Procesa Cola)          │
└──────┬────────────────────┘
       │
       ▼
┌───────────────────────────┐
│ Actualiza_Modulo_         │
│   Accounting()            │
└──────┬────────────────────┘
       │
       ▼
┌───────────────────────────┐
│  HTTP POST al Servidor    │
│  /api/Tarjeta/            │
│  ProcesarContadoresApi    │
│  Port: 9090               │
└───────────────────────────┘
```

## Dependencias Externas

- `ESP32Time`: Gestión de tiempo real
- `Contadores_SAS`: Fuente de datos de contadores
- `Variables_Globales`: Flags y estado global
- `Configuracion_ESP32`: Configuración de red y máquina
- `WiFiClient` / `HTTPClient`: Comunicación HTTP
- `ArduinoJson`: Serialización/deserialización JSON

## Consideraciones de Diseño

### Naturaleza Acumulativa de Contadores
- **Diseño Intencional**: Los contadores son valores acumulativos, no snapshots instantáneos
- **Ventaja**: Elimina la necesidad de mecanismos complejos de persistencia y reintento
- **Tolerancia a Fallos**: Si una transmisión falla, la próxima envía el valor correcto
- **Diferencia con Premios**: A diferencia del módulo Cashless (que maneja premios individuales críticos), Accounting no requiere persistencia local

### Patrón Productor-Consumidor
- **Productor**: Diversos módulos generan solicitudes de contabilidad
- **Cola**: `QueueAccounting` (capacidad 3 elementos)
- **Consumidor**: `Task_Accounting()` procesa requests
- **Nota**: La capacidad limitada de la cola es aceptable debido al carácter acumulativo

### Timeout No Bloqueante
- `Task_Accounting()` usa timeout 0 en `xQueueReceive()`
- Permite polling sin bloquear la tarea

## Bugs y Limitaciones Conocidas

### 1. Bug en `Transmite_Contadores_Api_Cola()` (líneas 83-101)
```cpp
AccountingRequest_t request;
// No se asignan valores a request.Com, request.IsSuccess, request.Message
if (xQueueSend(QueueAccounting, &request, 0) != pdPASS)
```
Los parámetros `Comando`, `IsOk`, y `Messages` se reciben pero **no se asignan** al request antes de enviarlo a la cola.

### 2. Valores Fijos en `Task_Accounting()` (líneas 103-121)
Siempre llama a `Actualiza_Modulo_Accounting()` con:
- Comando: 3 (fijo)
- IsSuccess: true (fijo)
- Message: "Solicitud actualizar Modulo Accounting" (fijo)

Esto significa que los valores originales del request **no se utilizan**.

### 3. Capacidad Limitada de Cola
Solo 3 elementos pueden estar en cola simultáneamente.

## Configuración

### Endpoint del Servidor
- **Contadores**: `http://[IP_Server]:9090/api/Tarjeta/ProcesarContadoresApi`

### Timeout
- **HTTP Contadores**: 10 segundos

## Ejemplo de Uso

### Inicialización
```cpp
API_Accounting Accounting;
Accounting.Init_Cola_Accounting();
```

### Enviar Contadores
```cpp
Accounting.Transmite_Contadores_Api_Cola(3, true, "Actualización periódica");
```

### Procesar Cola (llamado periódicamente en el loop principal)
```cpp
Accounting.Task_Accounting();
```

## Impacto

### Impacto Operacional
- **Transmisión de Contadores**: Es el canal principal de comunicación de contadores acumulativos hacia el backend
- **Auditoría**: Los datos transmitidos son críticos para auditoría, con **tolerancia a fallos parciales** por naturaleza acumulativa
- **Disponibilidad**: Si el módulo falla temporalmente, la siguiente transmisión exitosa sincronizará todos los valores
- **Performance**: El timeout de 10 segundos puede afectar la frecuencia de actualización, pero no la integridad final

### Impacto en Negocio
- **Pérdida de Datos**: **No crítico** - Los contadores son acumulativos, la siguiente transmisión contiene el valor correcto
- **Inconsistencias**: El bug de parámetros no asignados solo afecta `IsSuccess` y `Message`, no los valores de contadores
- **Costos**: Transmisiones fallidas consumen ancho de banda pero no afectan la integridad final de datos
- **Compliance**: El diseño acumulativo asegura que los datos correctos lleguen eventualmente al servidor

### Impacto Técnico
- **Memoria**: Cola de 3 elementos × sizeof(AccountingRequest_t) ≈ 150 bytes
- **CPU**: Procesamiento asíncrono reduce carga en el loop principal
- **Red**: Tramas JSON de ~2.5KB por transmisión
- **RTOS**: Utiliza 1 cola del sistema FreeRTOS

## Componentes Afectados

### Dependencias Directas
| Componente | Tipo | Descripción |
|------------|------|-------------|
| `Contadores_SAS` | Clase | Fuente de datos de contadores |
| `Variables_Globales` | Clase | Flags de estado (Flag_Maquina_En_Juego, etc.) |
| `Configuracion_ESP32` | Clase | Configuración IP e ID de máquina |
| `ESP32Time` | Librería | Timestamps para Fecha_Hora |
| `WiFiClient/HTTPClient` | Librería | Comunicación HTTP |
| `ArduinoJson` | Librería | Serialización JSON |
| `Info_Cashless` | Objeto externo | Tokens y hash para autenticación |

### Módulos que Consumen API_Accounting
| Módulo | Forma de Uso |
|--------|--------------|
| `Maquina_Estados` | Llama `Transmite_Contadores_Api_Cola()` en eventos de estado |
| `Contadores` | Actualiza valores que se transmiten |
| `RFID` | Actualiza ID_Jugador e Id_Operador |
| `main.cpp` | Inicializa cola y llama `Task_Accounting()` periódicamente |

### Archivos Relacionados
```
include/
├── API_Accounting.h          # Declaración de clase
├── Contadores.h              # Definición de contadores
├── Clase_Variables_Globales.h # Flags globales
└── Config_Perifericos.h      # Configuración

src/
├── API_Accounting.cpp        # Implementación
├── Contadores.cpp            # Gestión de contadores
├── Clase_Variables_Globales.cpp # Variables globales
└── main.cpp                  # Inicialización y loop

docs/
└── API_ACCOUNTING.md         # Esta documentación
```

## Riesgos Técnicos y de Seguridad

### Riesgos Técnicos

#### 1. Naturaleza Acumulativa de Contadores **[CARACTERÍSTICA DE DISEÑO]**
- **Descripción**: Los contadores son valores acumulativos, no instantáneos
- **Comportamiento**: Si una transmisión falla, la siguiente enviará el valor correcto acumulado
- **Impacto**: El sistema es **tolerante a pérdidas parciales** de transmisión
- **Ventaja**: No requiere mecanismos complejos de persistencia o reintento
- **Nota**: El servidor recibe el valor correcto en la siguiente transmisión exitosa

#### 2. Timeout de 10 Segundos **[BAJO]**
- **Descripción**: Timeout HTTP fijo de 10 segundos
- **Impacto**: Conexiones lentas pueden fallar, pero próxima transmisión tendrá datos actualizados
- **Mitigación**:
  - Dado el carácter acumulativo, no es crítico
  - Opcional: Implementar timeout adaptativo si se requiere mayor frecuencia
- **Estado**: Aceptable para el diseño actual

#### 3. Bug de Parámetros No Asignados **[MEDIO]**
- **Descripción**: Los parámetros de `Transmite_Contadores_Api_Cola()` no se usan
- **Impacto**: El servidor siempre recibe valores fijos en `IsSuccess` y `Message`
- **Nota**: Los **contadores numéricos sí se transmiten correctamente** (son valores acumulados)
- **Mitigación**:
  ```cpp
  // Corrección recomendada para completitud:
  AccountingRequest_t request;
  request.Com = Comando;
  request.IsSuccess = IsOk;
  request.Message = Messages;
  ```
- **Estado**: Mejora recomendada (no crítico para valores de contadores)

#### 4. Capacidad Limitada de Cola (3 elementos) **[BAJO]**
- **Descripción**: Cola de FreeRTOS con capacidad de 3 elementos
- **Impacto**: Solicitudes adicionales se descartan, pero no afecta valores acumulados
- **Nota**: Dado que los contadores son acumulativos, el descarte no implica pérdida de datos
- **Mitigación**:
  - Actualmente suficiente para el patrón de uso
  - Opcional: Aumentar si se requiere mayor frecuencia de actualización
- **Estado**: Aceptable para el diseño actual

### Riesgos de Seguridad

#### 1. Naturaleza Acumulativa y No-crítica de Transmisiones **[CARACTERÍSTICA DE DISEÑO]**
- **Descripción**: No existe persistencia local de transmisiones fallidas
- **Impacto**: **NULO** - Al ser contadores acumulativos, no es necesario
- **Ventaja**: Simplifica el diseño y reduce uso de memoria/SPIFFS
- **Nota**: A diferencia del módulo de premios (que sí requiere persistencia), los contadores se recuperan en la siguiente transmisión
- **Estado**: Diseño intencional y correcto

#### 2. Transmisión HTTP sin Encriptación **[CRÍTICO]**
- **Descripción**: Comunicación vía HTTP (no HTTPS)
- **Impacto**: Contadores y datos de máquina son transmitidos en texto plano
- **Mitigación**:
  - Implementar HTTPS con certificados
  - O utilizar túnel VPN/SSH
- **Estado**: Requiere acción inmediata

#### 2. Tokens en Memoria **[MEDIO]**
- **Descripción**: Tokens de autenticación almacenados en RAM
- **Impacto**: Si el dispositivo es comprometido físicamente, los tokens pueden ser extraídos
- **Mitigación**:
  - Considerar almacenamiento en secure element
  - Rotación periódica de tokens
- **Estado**: Buenas prácticas recomendadas

#### 3. Falta de Validación de Respuesta **[BAJO]**
- **Descripción**: Se valida solo `IsSuccess` del JSON de respuesta
- **Impacto**: Respuestas malformadas podrían causar comportamientos inesperados
- **Mitigación**:
  - Validar estructura completa de respuesta
  - Implementar schema validation
- **Estado**: Mejora recomendada

#### 4. Exposición de MAC Address **[BAJO]**
- **Descripción**: MAC address transmitido en cada solicitud
- **Impacto**: Permite rastreo físico del dispositivo
- **Mitigación**:
  - Considerar hash de MAC en lugar de valor crudo
  - O generar ID único aleatorio
- **Estado**: Mejora opcional

### Riesgos de Cumplimiento (Compliance)

#### Auditoría Fiscal
- **Riesgo**: Pérdida de contadores puede afectar reportes fiscales
- **Recomendación**: Implementar logging inmutable de todas las transmisiones

#### Trazabilidad
- **Riesgo**: Falta de correlación entre eventos locales y servidor
- **Recomendación**: Agregar ID de transacción único para correlación

## Historial de Cambios

- **v2.0**: Implementación inicial con colas FreeRTOS
- **Correcciones pendientes**: 
  - Asignación de parámetros en `Transmite_Contadores_Api_Cola()` (mejora, no crítico)
  - Uso de valores del request en `Task_Accounting()` (mejora, no crítico)
- **Mejoras pendientes**:
  - Implementación de HTTPS para seguridad en transmisión
  - Opcional: Aumentar capacidad de cola si se requiere mayor frecuencia de actualización
