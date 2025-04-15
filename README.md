# STM32 Custom Serial Communication Project

This project was created during my junior year as part of a three-person team. It demonstrates how to build a custom serial communication protocol from the ground up using the STM32F4 microcontroller. The system sends and receives messages using GPIO, timers, and interrupt-driven logic, all without relying on built-in UART peripherals for the actual protocol.

## Project Overview

The system allows for sending and receiving messages between STM32 devices using a custom protocol. It includes basic networking features like:

- Framing with preamble, source, destination, length, CRC, and trailer
- Collision detection and retransmission using a random backoff timer
- Visual feedback via onboard LEDs for states like idle, busy, and collision
- A user interface over USART2 for sending (`send`) and receiving (`r`) messages

## Key Features

- Bit-level framing and Manchester-style encoding
- Real-time message decoding using timer interrupts
- Retransmission logic for message delivery reliability
- Simple command-line interface via USB serial connection
- Custom handling of message length, address validation, and CRC checks

## Technologies Used

- STM32F4 microcontroller (Cortex-M4)
- STM32CubeIDE
- C programming (bare-metal)
- Timers (TIM2, TIM3, TIM4)
- USART2 (for user input/output)

## How to Use

1. Flash the project to your STM32 board using STM32CubeIDE.
2. Connect to the board via serial terminal (57600 baud).
3. Type `send <address> <message>` to send data.
4. Type `r` to attempt to receive a message.

### Example

```
send 0x3F Hello World
r
Hello World
```

## File Breakdown

- `main.c`: User interface and command handling
- `monitor.c/h`: Receiver logic using TIM2
- `transmitter.c/h`: Transmission logic and retries
- `uart_driver.c/h`: USART setup for user I/O
- `regs.h`: Register definitions for STM32 hardware
- `Milestone2VER1.pdf`: Documentation and original project write-up

## Team Members

- Troy Wypishinski-Prechter (me)
- Matthew Hampel  
- [Add third teammate here if you'd like]

## Notes

This was a great learning experience in embedded systems, especially for working with hardware timers, handling bit-level protocols, and creating interrupt-driven systems without an RTOS. Future additions could include better CRC validation, support for larger message sizes, or a full debugging interface.

---
