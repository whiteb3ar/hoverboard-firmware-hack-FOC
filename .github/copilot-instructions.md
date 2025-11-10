# AI Agent Instructions for Hoverboard FOC Firmware

## Project Overview
This is a Field Oriented Control (FOC) firmware for stock hoverboards, implementing advanced motor control with features like reduced noise, smooth torque output, and field weakening capabilities. The project targets STM32F103RCT6/GD32F103RCT6 mainboards found in typical hoverboards.

## Key Architecture Components
- **Motor Control Core** (`lib/foc/`): BLDC motor controller implementation
  - `BLDC_controller.c/h`: Core FOC algorithm
  - `BLDC_controller_data.c`: Calibrated motor parameters in fixed-point format
- **Hardware Abstraction** (`src/hal/`): STM32 platform-specific code
- **Communication** (`src/comms.c`): Handles various input protocols (UART, PWM, PPM, iBUS)
- **Core Logic** (`src/`):
  - `main.c`: Main loop and initialization
  - `bldc.c`: Motor interface
  - `control.c`: Input processing and motor control mapping

## Build System & Variants
- Uses PlatformIO for builds (`platformio.ini`)
- Multiple build variants available:
  - VARIANT_ADC: Control via ADC input
  - VARIANT_USART: Serial control
  - VARIANT_NUNCHUK: Nunchuk controller
  - VARIANT_PPM/PWM: RC remote control
  - VARIANT_HOVERBOARD: Standard hoverboard
  - Others: HOVERCAR, TRANSPOTTER, SKATEBOARD

To build a variant:
1. Select variant in `platformio.ini` under `default_envs`
2. Build using PlatformIO commands

## Critical Configuration
Key settings in `include/config.h`:
- Motor parameters and timing (`PWM_FREQ`, `DEAD_TIME`)
- Battery monitoring thresholds and cell count
- ADC timing and conversion settings
- Board variant pin mappings

## Control Modes
Three main control types (set via `CTRL_TYP_SEL` in `config.h`):
1. Commutation (COM_CTRL)
2. Sinusoidal (SIN_CTRL)
3. Field Oriented Control (FOC_CTRL) with modes:
   - Voltage Mode (VLT_MODE)
   - Speed Mode (SPD_MODE)
   - Torque Mode (TRQ_MODE)

## Development Workflow
1. Always calibrate parameters using Fixed-Point Viewer tool for motor parameters
2. Test changes incrementally with motor safety protections enabled
3. Monitor battery voltage and temperature thresholds during testing

## Integration Points
- USART2/3: Main external interfaces (left/right sideboard connectors)
  - Support UART, PWM, PPM, iBUS
  - USART2: 12-bit ADC capability (not 5V tolerant)
  - USART3: I2C capability (5V tolerant)

## Safety Considerations
1. Never disable motor current and speed protections
2. Validate battery cell count and voltage thresholds
3. Field weakening changes require safety measures (motors can spin very fast)
4. Test new control parameters with motors secured