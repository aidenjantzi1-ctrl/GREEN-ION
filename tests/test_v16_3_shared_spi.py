from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=(ROOT/"Green_Ion_V16_3/Green_Ion_V16_3.ino").read_text(encoding="utf-8")
CFG=(ROOT/"Green_Ion_V16_3/Config.h").read_text(encoding="utf-8")
HOME=(ROOT/"Green_Ion_V16_3/HomeVerification.h").read_text(encoding="utf-8")
UNO=(ROOT/"Ground_Receiver_V16_3/Ground_Receiver_V16_3.ino").read_text(encoding="utf-8")
PROTO=(ROOT/"libraries/GreenIonProtocol/src/GreenIonProtocol.h").read_text(encoding="utf-8")
SPI=(ROOT/"libraries/GreenIonProtocol/src/GreenIonSharedSpi.h").read_text(encoding="utf-8")

class V163SharedSpiTests(unittest.TestCase):
    def test_v163_identity_and_pins_preserved(self):
        self.assertIn('FIRMWARE_NAME[] = "GREEN ION Feather V16.3"',FW)
        self.assertIn('FIRMWARE_VERSION[] = "16.3.0"',FW)
        self.assertIn('"GREEN-ION-V16-3"',CFG)
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "16.3.0"',UNO)
        expected={
          "PIN_SPI_SCK":"5","PIN_SPI_MOSI":"19","PIN_SPI_MISO":"21",
          "PIN_SD_CS":"33","PIN_LORA_CS":"27","PIN_LORA_RST":"15","PIN_LORA_DIO0":"32",
        }
        for name,value in expected.items():
            self.assertRegex(CFG,rf"{name}\s*=\s*{value}\s*;")

    def test_file_and_radio_formats_remain_backward_compatible(self):
        self.assertIn('static const uint8_t kProtocolVersion = 2;',PROTO)
        self.assertIn('static_assert(sizeof(TelemetryPacket) == 70',PROTO)
        self.assertIn('"/V12D%03u.BIN"',FW)
        self.assertIn('#include <GreenIonRkt4.h>',FW)
        self.assertIn('#include <GreenIonSensorLog.h>',FW)
        self.assertNotIn("STATE_IGNITION",PROTO)
        self.assertNotIn("parachute",PROTO.lower())

    def test_radiohead_spi_is_deferred_out_of_dio0_isr(self):
        self.assertIn("RH_RF95(chipSelect, RH_INVALID_PIN)",FW)
        self.assertIn("void serviceDeferredInterrupt() { handleInterrupt(); }",FW)
        match=re.search(r"static void IRAM_ATTR radioDio0Isr\(\)\s*\{([^}]*)\}",FW,re.S)
        self.assertIsNotNone(match)
        body=match.group(1)
        self.assertIn("++radioIrqGeneration",body)
        for forbidden in ("radio.","SPI.","SD.","digitalWrite","handleInterrupt","serviceDeferredInterrupt"):
            self.assertNotIn(forbidden,body)
        self.assertIn("attachInterrupt(digitalPinToInterrupt(config::PIN_LORA_DIO0)",FW)
        self.assertIn("serviceDeferredRadioInterrupt(nowMs, micros())",FW)

    def test_deferred_radio_service_respects_recorder_deadline_and_spi_owner(self):
        start=FW.index("static void serviceDeferredRadioInterrupt")
        end=FW.index("static void serviceRadioRecovery",start)
        block=FW[start:end]
        self.assertIn("hasDeadlineMargin",block)
        self.assertIn("RADIO_IRQ_SERVICE_GUARD_US",block)
        self.assertIn("RadioSpiScope bus",block)
        self.assertIn("radio.serviceDeferredInterrupt()",block)
        self.assertIn("radioTxTimeouts",block)
        self.assertIn("radioTxAwaitingCompletion",block)
        self.assertIn("recorder_unchanged=1",block)

    def test_transmit_start_is_not_misreported_as_completed(self):
        start=FW.index("static void serviceTelemetry")
        end=FW.index("void setup()",start)
        block=FW[start:end]
        self.assertIn("radio.send",block)
        self.assertIn("++radioTxStarted",block)
        self.assertIn("radioTxAwaitingCompletion = true",block)
        self.assertNotIn("++radioBenchPacketsSent",block)
        irq=FW[FW.index("static void serviceDeferredRadioInterrupt"):FW.index("static void serviceRadioRecovery")]
        self.assertIn("++radioTxCompleted",irq)
        self.assertIn("++radioBenchPacketsSent",irq)

    def test_sd_and_radio_scopes_never_toggle_the_active_devices_chip_select(self):
        self.assertIn("class SdSpiScope",FW)
        self.assertIn("class RadioSpiScope",FW)
        sd=FW[FW.index("class SdSpiScope"):FW.index("class RadioSpiScope")]
        radio=FW[FW.index("class RadioSpiScope"):FW.index("static const char FIRMWARE_NAME")]
        self.assertIn("PIN_LORA_CS, HIGH",sd)
        self.assertNotIn("PIN_SD_CS, HIGH",sd)
        self.assertIn("PIN_SD_CS, HIGH",radio)
        self.assertNotIn("PIN_LORA_CS, HIGH",radio)
        # Outside the scope helper and boot-wide deselect helper there must be
        # no ad-hoc inactive-radio CS writes.
        self.assertEqual(FW.count("digitalWrite(config::PIN_LORA_CS, HIGH);"),2)

    def test_shared_spi_coordinator_is_bounded_and_nonblocking(self):
        self.assertIn("OWNER_IDLE",SPI)
        self.assertIn("radioDeferredBusy",SPI)
        self.assertIn("radioDeferredDeadline",SPI)
        self.assertIn("ownershipErrors",SPI)
        self.assertNotIn("while (",SPI)
        self.assertNotIn("delay(",SPI)

    def test_home_test_exercises_sd_and_optional_lora_concurrently(self):
        self.assertIn("HOME_CONCURRENT_RADIO_PACKETS = 8",CFG)
        self.assertIn("radioBenchPacketsRemaining = config::HOME_CONCURRENT_RADIO_PACKETS",HOME)
        self.assertIn("homeRadioConcurrentExpected = state.radioOk",HOME)
        self.assertIn("50 Hz SD + optional LoRa shared-SPI stress test",HOME)
        self.assertIn("home_shared_spi_result",FW)
        for field in (
          "home_radio_expected","home_radio_target","home_telemetry_dropped",
          "home_sensor_log_dropped","home_spi_ownership_errors","home_radio_tx_timeouts"
        ):
            self.assertIn(field,FW)

    def test_status_exposes_shared_spi_evidence_without_claiming_electrical_contention(self):
        for field in (
          "shared_spi_owner","shared_spi_sd_sections","shared_spi_radio_sections",
          "shared_spi_radio_deferred_busy","shared_spi_radio_deferred_deadline",
          "shared_spi_ownership_errors","radio_irq_observed","radio_irq_serviced",
          "radio_irq_deferred","radio_tx_started","radio_tx_completed",
          "radio_tx_timeouts","radio_tx_awaiting_completion","radio_last_tx_age_ms"
        ):
            self.assertIn(field,FW)
        self.assertIn("HEALTH_SHARED_SPI_OWNERSHIP_ERROR",PROTO)
        self.assertIn("HEALTH_RADIO_TX_TIMEOUT",PROTO)
        self.assertNotIn("electrical_contention",FW)

if __name__=="__main__":
    unittest.main()
