# WULPUS PRO Full Specifications

## Transducer and transmit path

| Feature | Specification |
| --- | --- |
| Channels | 16, time-multiplexed |
| Transducer support | PZT transducers and CMUTs |
| Transducer bias | Indirect or direct bias, -30 V or 30 V |
| Excitation amplitude | 30 V unipolar |
| Excitation frequency | 100 kHz to 10 MHz |

## Receive path

| Feature | Specification |
| --- | --- |
| Analog front-end | 6 dB LNA + 70 dB VGA |
| Time gain compensation | TGC up to 70 dB depth-dependent attenuation compensation |
| Envelope extraction | Optional LTC5507-based envelope detector, runtime configurable |
| Envelope detector bandwidth | Up to 1.5 MHz envelope bandwidth |
| Amplification-path bandwidth | 9.9 MHz |
| End-to-end -3 dB bandwidth | 1.4 MHz (bounded by MSP430) |
| Usable measured bandwidth | SNR >=10 dB up to 2.5 MHz |
| Passband SNR | Approximately 32 dB at 40 dB total gain |
| Peak SINAD | 41.42 dB (1 MHz), 36.25 dB (2.25 MHz), both at 4.9 mV input amplitude |
| ENOB | ~7.8 bits |

## Acquisition and data link

| Feature | Specification |
| --- | --- |
| ADC | 8 Msps analog-to-digital converter, 12-bit resolution |
| Host interface | SPI, 8 MHz |
| Maximum PRF | 300 Hz |
| Raw streaming over BLE | 50 Hz PRF |
| Raw streaming over Wi-Fi | Up to 300 Hz PRF |

## Power

| Feature | Specification |
| --- | --- |
| Power budget | <=40 mW at 50 Hz PRF |
| Core electronics power | 35 mW (50 Hz PRF), 58 mW (300 Hz PRF) |
| Battery-life reference | More than 24 hours of continuous raw data streaming at 50 Hz PRF from a 300 mAh Li-Po battery with BLE |

## Imaging performance

| Feature | Specification |
| --- | --- |
| B-mode frame rate | 18 FPS using 16-channel synthetic-aperture acquisition at 300 Hz PRF |
| B-mode axial resolution | ~ 0.7 mm with a 2.25 MHz transducer (LA-2.25-32, Vermon) |
| B-mode lateral resolution | ~ 2.3 mm at the center with a 2.25 MHz transducer (LA-2.25-32, Vermon) |

## Mechanics

| Feature | Specification |
| --- | --- |
| Module size | 39 x 21 x 6 mm |
| Weight | 5 g |
