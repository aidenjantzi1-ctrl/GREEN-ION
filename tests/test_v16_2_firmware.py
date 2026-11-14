from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=(ROOT/"Past_Versions/V16/Green_Ion_V16_2/Green_Ion_V16_2.ino").read_text(encoding="utf-8")
CFG=(ROOT/"Past_Versions/V16/Green_Ion_V16_2/Config.h").read_text(encoding="utf-8")
STATE=(ROOT/"Past_Versions/V16/Green_Ion_V16_2/SensorState.h").read_text(encoding="utf-8")
UNO=(ROOT/"Past_Versions/V16/Ground_Receiver_V16_2/Ground_Receiver_V16_2.ino").read_text(encoding="utf-8")
PROTOCOL=(ROOT/"libraries/GreenIonProtocol/src/GreenIonProtocol.h").read_text(encoding="utf-8")

class V162FirmwareTests(unittest.TestCase):
    def test_new_generation_is_explicit(self):
        self.assertIn('FIRMWARE_NAME[] = "GREEN ION Feather V16.2"',FW)
        self.assertIn('FIRMWARE_VERSION[] = "16.2.0"',FW)
        self.assertIn('"GREEN-ION-V16-2"',CFG)
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "16.2.0"',UNO)
        self.assertIn('receiver_firmware',UNO)

    def test_binary_flight_formats_and_radio_airtime_contract_are_unchanged(self):
        self.assertIn('static const uint8_t kProtocolVersion = 2;',PROTOCOL)
        self.assertIn('static_assert(sizeof(TelemetryPacket) == 70',PROTOCOL)
        self.assertIn('"/V12D%03u.BIN"',FW)
        self.assertIn('#include <GreenIonRkt4.h>',FW)
        self.assertIn('#include <GreenIonSensorLog.h>',FW)
        self.assertNotIn('STATE_IGNITION',PROTOCOL)
        self.assertNotIn('deploy',PROTOCOL.lower())

    def test_status_exposes_truthful_firmware_and_recorder_diagnostics(self):
        for token in (
            '"firmware_name"', '"firmware_version"', '"radio_protocol"',
            '"boot_session_id"', '"reset_reason"', '"sd_mount_attempts"',
            '"sd_recoveries"', '"imu_recoveries"', '"gps_recoveries"',
            '"telemetry_queue_depth"', '"telemetry_queue_dropped"',
            '"barometer_read_age_ms"', '"imu_read_age_ms"', '"battery_read_age_ms"',
            '"missed_sample_periods"', '"sensor_log_dropped"'
        ):
            self.assertIn(token.replace('"','\\"') if token.startswith('"') else token,FW)

    def test_recovery_work_is_idle_only(self):
        self.assertIn("static bool storageRecoveryAllowed()",FW)
        start=FW.index("static bool storageRecoveryAllowed()")
        end=FW.index("static void requestHardwareTest",start)
        block=FW[start:end]
        for token in ("!state.recording","!recorderCore.recording()","!logFile","!sensorLogFile",
                      "RECORDER_RECORDING","RECORDER_HOME_TEST","RECORDER_MAINTENANCE","otaUploadInProgress"):
            self.assertIn(token,block)
        self.assertIn("Existing recorder fault is not auto-cleared",FW)
        self.assertIn("serviceStorageRecovery(nowMs)",FW)
        self.assertIn("servicePeripheralRecovery(nowMs)",FW)

    def test_metadata_additions_do_not_replace_rkt4_or_gis1(self):
        self.assertIn('metadataFile.print("firmware_version="',FW)
        self.assertIn('metadataFile.print("boot_session_id="',FW)
        self.assertIn('metadataFile.print("reset_reason="',FW)
        self.assertIn("green_ion::rkt4::Header header",FW)
        self.assertIn("green_ion::sensor_log::Header sensorHeader",FW)

    def test_diagnostic_health_bits_fit_existing_16_bit_field(self):
        for bit in range(8,12):
            self.assertRegex(PROTOCOL,rf"1u << {bit}")
        self.assertNotRegex(PROTOCOL,r"1u << (1[6-9]|[2-9][0-9])")

if __name__=="__main__":
    unittest.main()
