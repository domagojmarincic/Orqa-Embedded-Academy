# Task 2 — SPI NOR Flash Driver

**ORQA Embedded Academy · MiniUpdater, stage 1**

**Due:** pull request opened before Thursday 03 August
**Milestone:** your driver, through your solder joints, passes the full suite on real hardware.

---

## 1. Why this task exists

MiniUpdater has to hold a firmware image across a power cycle. That means a W25Q NOR flash on the SPI bus, and that means somebody has to write the driver. This week that somebody is you.

Everything later in the academy sits on top of this: FatFS mounts on it in Week 2, the updater streams out of it in Week 3, and a task reads from it in Week 4. 

## 2. What you get

```
Docs/                            W25Q512JVEIQ datasheet + ORQA Academy Flash Board schematic
Drivers/spif/spif.h              public interface — do not change the signatures
Drivers/spif/incl/               the two internal layers, declarations only
Drivers/spif/impl/               empty stubs, every function returns false
Tests/spif_tests/                the test suite — provided, complete, and not yours to edit
Core/                            CubeMX project: USART2 up, printf wired, SPI *not* configured
```

## 3. The shape of the task

**The tests are the specification.** `Tests/spif_tests/spif_tests.c` is written, it is complete, and it defines exactly what "done" means. Your job is to make it print zero failures. Every question you have about the expected behaviour is answered somewhere in that file.

Three consequences worth understanding:

- **You do not edit the tests.** Not to loosen a check, not to skip a case. `SPIF_TEST_BLOCK` and `SPIF_TEST_ERASE_CHIP` in `spif_tests.h` are the only knobs. A PR that touches the assertions is a failed PR.
- **The suite never uses the driver to check the driver.** It carries its own small reference implementation that talks to the chip straight through the HAL. A driver that is wrong the same way when reading and when writing still fails, which is the whole point.
- **The suite tests all three layers**, not just the public API. `spif_utils` is checked offline, `spif_commander` is checked on the wire, `spif` is checked end to end. That is why the driver is split into three files, and why you should keep it that way.

## 4. The driver structure

| File | Holds |
|---|---|
| `impl/spif_utils.c` | Pure logic: readiness, range checks, region clamping, decoding the JEDEC capacity byte. No hardware. |
| `impl/spif_commander.c` | Everything that touches the bus: chip select, raw transmit and receive, single-byte commands, command-plus-address, status register, busy polling. |
| `impl/spif.c` | The public API, built out of the two layers above. |

Add private static helpers inside any of these files if you need them. Do not add anything to the headers — the declarations there are the full internal API, and they are what the suite links against.

**The RTOS switch.** `spif.h` carries a `SPIF_USE_RTOS` flag and, in the handle, either a CMSIS-RTOS mutex or a plain bare-metal guard depending on it. We have not covered FreeRTOS yet and you do not need to this week — the flag stays at `0`, only the bare-metal branch compiles, and that is the branch the suite exercises. What you do need is to respect the shape it implies: every public call takes the guard before it touches the bus and releases it on every exit path, failures included, and every busy-wait goes through one place. 

**Suggested order of attack.** `spif_utils` first: four functions, no hardware, four tests turn green on the desk before you plug anything in. Then `spif_commander`, with the logic analyzer running. Then `spif`, where the interesting behaviour lives.

## 5. Step 1 — Bring up the bus from the documentation

**Nothing SPI-related is configured in the project.** You enable it, and `Docs/` is where the answers are.

`Docs/embedded_academy_board.pdf` is the schematic of the flash board you soldered — an ORQA Academy Flash Board v1.1.1 carrying a `W25Q512JVEIQ`, which plugs onto the Nucleo morpho headers CN7 and CN10. Read it to find which connector pins carry `CS`, `SCK`, `MOSI` and `MISO`, and note what happens to `WP` and `HOLD`. Cross-reference those connector pins against the NUCLEO-G0B1RE pinout to get MCU pins, and from the MCU pins the SPI peripheral and its alternate function.

`Docs/W25Q512JVEIQ.pdf` tells you the rest: which SPI mode the chip accepts, the maximum clock, the command table, the status register, and the timing limits you must not exceed.

Then, in CubeMX:

- Enable the SPI peripheral in full-duplex master mode, on the pins the schematic dictates.
- Set clock polarity and phase to a mode the chip actually supports, and pick a prescaler that stays inside its clock rating.
- Configure the chip select as a plain GPIO output. Software NSS, not hardware. **Label it `SPI2_CS`** so the code in `main.c` compiles as written.
- Think about what level the chip select must sit at when nothing is happening, and make sure the generated init code actually leaves it there.

Regenerate, then uncomment the two blocks marked `TODO Step 1` in `main.c`.

## 6. Step 2 — Monitoring over USART2

Test output goes out over **USART2**, which is routed to the ST-Link Virtual COM Port, so a single USB cable carries both the debugger and the log. It is configured for **115200 8N1** and `printf()` is already redirected to it by the `_write()` override at the bottom of `main.c`.

Open the port in the CubeIDE terminal, or in any serial terminal on the laptop. You should see the banner at reset before you write a line of driver code — if you do not, fix that first, because from here on the serial log is the only thing telling you whether you are passing.

The suite prints one `PASS` / `FAIL` line per test and a count at the end. That log is the artifact: paste the final run into your pull request.

## 7. Step 3 — Work test by test

The loop, once per test:

1. **Red.** Run the suite. Pick the first failing test. Read it — not the name, the body.
2. **Green.** Write the smallest amount of driver code that makes that one test pass, and check that nothing that was passing has stopped.
3. **Commit.** One commit per test turning green, with the test name in the message.
4. Repeat.

That commit history is what Monday's review reads. Fifteen small commits marching down the suite tells us more about how you work than a single "implement driver" drop, and it is much easier to help you from.

Do not chase the whole suite at once, and do not start with the hardest test. Green things stay green; that is the safety net.

## 8. Definition of done

- The suite prints zero failures on your board, through your solder joints.
- It passes twice in a row with a power cycle in between.
- `spif.h`, `incl/spif_utils.h`, `incl/spif_commander.h` and everything under `Tests/` are unchanged.
- Your PR is open before Monday 09:00.

## 9. Submission

Branch, commit as you go, and open a pull request against `develop` titled `feature/DZ2`. In the README.MD description:

- the final suite output;
- how you configured the SPI bus, and which pins you landed on;
- one thing the datasheet told you that you would not have guessed;
- where you got stuck, what you tried, and how it ended — including the dead ends.

That last point is not a formality. Monday's session is a stuck-point autopsy, and an honest debugging story scores better than silence.
Nobody is expected to arrive finished.

