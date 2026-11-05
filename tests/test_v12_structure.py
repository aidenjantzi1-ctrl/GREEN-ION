from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class V12StructureTests(unittest.TestCase):
    def test_required_projects_and_docs_exist(self):
        paths = (
            "Green_Ion_V12/Green_Ion_V12.ino", "Green_Ion_V12/Config.h",
            "Green_Ion_V12/Dashboard.h",
            "Ground_Receiver/Ground_Receiver.ino", "Ground_Station/ground_station.py",
            "Ground_Station/index.html", "Ground_Station/OPEN_GROUND_STATION.cmd",
            "Ground_Station/sensor_log.py",
            "libraries/GreenIonRecorder/src/GreenIonRecorderCore.h",
            "libraries/GreenIonRecorder/src/GreenIonFlightWatcher.h",
            "libraries/GreenIonRecorder/src/GreenIonCalibration.h",
            "libraries/GreenIonRecorder/src/GreenIonCheckpoint.h",
            "libraries/GreenIonRecorder/src/GreenIonSampleClock.h",
            "libraries/GreenIonRecorder/src/GreenIonMetadata.h",
            "libraries/GreenIonRecorder/src/GreenIonLandingWatcher.h",
            "libraries/GreenIonRecorder/src/GreenIonSensorLog.h",
            "libraries/GreenIonRecorder/src/GreenIonSensorLogCore.h",
            "libraries/GreenIonRecorder/src/GreenIonSensorRecovery.h",
            "libraries/GreenIonRecorder/src/GreenIonSensorHealth.h",
            "libraries/GreenIonRecorder/src/GreenIonBenchTests.h",
            "libraries/GreenIonProtocol/src/GreenIonTelemetryQueue.h",
            "libraries/GreenIonProtocol/src/GreenIonRadioRecovery.h",
            "tests/cpp/test_v12_recorder_core.cpp",
            "tests/cpp/test_v12_flight_watcher.cpp",
            "tests/cpp/test_v12_calibration.cpp",
            "tests/cpp/test_v12_checkpoint.cpp",
            "tests/cpp/test_v12_sample_clock.cpp",
            "tests/cpp/test_v12_metadata.cpp",
            "tests/cpp/test_v12_landing_watcher.cpp",
            "tests/cpp/test_v12_telemetry_queue.cpp",
            "tests/cpp/test_v12_radio_recovery.cpp",
            "tests/cpp/test_v12_sensor_log.cpp",
            "tests/cpp/test_v12_sensor_log_core.cpp",
            "tests/cpp/test_v12_sensor_recovery.cpp",
            "tests/cpp/test_v12_recording_failures.cpp",
            "tests/test_v12_sensor_export.py",
            "docs/V12_BUILD_GUIDE.md",
            "docs/V12_WIRING.md", "docs/V12_TESTING.md",
            "docs/V12_DEVELOPMENT_LOG.md", "docs/V12_ROADMAP.md",
            "docs/V12_HARDWARE_BRINGUP.md",
            "docs/V12_FIRST_DAY_TEST.md",
        )
        for path in paths:
            self.assertTrue((ROOT / path).is_file(), path)

    def test_pin_assignments_are_centralized_and_safe(self):
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        self.assertIn("PIN_SD_CS = 33", config)
        self.assertIn("PIN_LORA_CS = 27", config)
        self.assertIn("PIN_I2C_POWER = NEOPIXEL_I2C_POWER", config)
        self.assertIn("PIN_SD_CS != PIN_LORA_CS", config)

    def test_both_radios_use_the_shared_configuration(self):
        transmitter = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        receiver = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        for name in ("kFrequencyMhz", "kSignalBandwidthHz", "kSpreadingFactor", "kCodingRateDenominator"):
            self.assertIn(name, transmitter)
            self.assertIn(name, receiver)

    def test_ground_station_opens_in_hardware_mode_without_arguments(self):
        source = (ROOT / "Ground_Station" / "ground_station.py").read_text(encoding="utf-8")
        self.assertIn('if args.sample:', source)
        self.assertIn('threading.Thread(target=serial_source', source)
        self.assertIn('threading.Thread(target=wifi_source', source)
        self.assertIn('webbrowser.open', source)
        self.assertNotIn('args.sample or not args.port', source)

    def test_firmware_uses_bounded_recorder_core(self):
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        self.assertIn("PREBUFFER_SECONDS = 3", config)
        self.assertIn("PREBUFFER_RECORDS == 150", config)
        self.assertIn("RecorderCore<config::PREBUFFER_RECORDS>", firmware)
        self.assertIn("class SdFileSink", firmware)
        self.assertIn("recorderCore.observe(record)", firmware)
        self.assertIn("RECORDER_CALIBRATING", firmware)
        self.assertIn("RECORDER_WATCHING", firmware)
        self.assertIn("processCalibration(nowMs, valid)", firmware)
        self.assertIn("updateLaunchWatcher(nowMs, valid)", firmware)
        self.assertIn("saveCheckpoint(nowMs)", firmware)
        self.assertIn("CHECKPOINT_INTERVAL_MS = 5000", config)
        self.assertIn('replace(".BIN", ".JRA")', firmware)
        self.assertIn('replace(".BIN", ".JRB")', firmware)
        self.assertIn("else if (nowMs - lastCheckpointMs", firmware)
        self.assertIn("recoverInterruptedLogs();", firmware)
        self.assertIn("selectNewestCheckpoint(", firmware)
        self.assertIn("commitRecovery(", firmware)
        self.assertIn("class SdRecoveryCommitter", firmware)
        self.assertIn("SampleClock sampleClock", firmware)
        self.assertIn("sampleClock.start(micros())", firmware)
        self.assertIn("sampleClock.due(nowUs)", firmware)
        self.assertNotIn("nowUs - lastSensorUs", firmware)
        self.assertIn("writeStartMetadata();", firmware)
        self.assertIn('metadataFilename.replace(".BIN", ".TXT")', firmware)
        self.assertIn("Metadata failure never stops the primary BIN recorder", firmware)
        self.assertIn("LandingWatcher landingWatcher", firmware)
        self.assertIn("landingWatcher.begin(nowMs", firmware)
        self.assertIn("landingWatcher.update(", firmware)
        self.assertIn("finalizeRecording(nowMs)", firmware)
        self.assertIn("recorderCore.stop()", firmware)
        self.assertIn("writeCompletionMetadata(nowMs", firmware)
        self.assertLess(
            firmware.index("recorderCore.stop()"),
            firmware.index("writeCompletionMetadata(nowMs"),
        )
        self.assertLess(
            firmware.index("writeCompletionMetadata(nowMs"),
            firmware.index("SD.remove(checkpointFilenameA)"),
        )
        self.assertLess(
            firmware.index("SD.remove(checkpointFilenameA)"),
            firmware.index("SD.remove(checkpointFilenameB)"),
        )
        self.assertIn('reportPath.replace(".BIN", ".RCV")', firmware)
        self.assertIn("trailing_partial_bytes", firmware)
        dashboard = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
        self.assertIn('id="recovery"', dashboard)
        self.assertIn("d.recovery_status", dashboard)
        self.assertIn('id="missed"', dashboard)
        self.assertIn("d.missed_sample_periods", dashboard)
        self.assertIn('id="metadata"', dashboard)
        self.assertIn("d.metadata_file", dashboard)
        self.assertIn("SD.open(binPath, FILE_READ)", firmware)
        self.assertNotIn("SD.remove(binPath)", firmware)
        self.assertNotIn("log.truncate", firmware)
        self.assertNotIn("state.sdOk && !recorderCore.recording()) openRawLog()", firmware)

    def test_no_energetic_device_control_surface(self):
        source = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for folder in (ROOT / "Green_Ion_V12", ROOT / "Ground_Receiver")
            for path in folder.rglob("*") if path.is_file()
        ).lower()
        for forbidden in ("fire_igniter", "/api/ignition", "deploy_parachute", "stage_separation"):
            self.assertNotIn(forbidden, source)

    def test_imu_and_gps_serial_bench_contract(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        state = (ROOT / "Green_Ion_V12" / "SensorState.h").read_text(encoding="utf-8")
        for token in (
            "BENCH IMU status=OK", "range=+-32g", "rate_hz=104",
            "accel_mps2=", "accel_g=", "gyro_rad_s=", "gyro_dps=",
            "total_g=", "temperature_c=", "range_verified=",
            "BENCH GPS status=%s", '"LIVE" : "STALE"',
            "bench imu | bench gps | bench all | bench status | bench off",
        ):
            self.assertIn(token, firmware)
        self.assertIn("state.imuDataValid = imu.getEvent", firmware)
        self.assertIn("state.gpsFixCurrent = gps.fix", firmware)
        self.assertIn("state.gpsHasPosition && state.gpsFixCurrent", firmware)
        self.assertIn("bool gpsFixCurrent = false", state)

    def test_nonblocking_hardware_test_dashboard_contract(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        dashboard = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
        bench = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonBenchTests.h").read_text(encoding="utf-8")
        for token in ("Test IMU", "Test GPS", "Test All Sensors", "/api/hardware-test"):
            self.assertIn(token, dashboard + firmware)
        for token in ("test imu", "test gps", "test all", "test status"):
            self.assertIn(token, firmware)
        self.assertIn("imu.getAccelRange() == LSM6DSO32_ACCEL_RANGE_32_G", firmware)
        self.assertIn("gpsNmeaSentences", firmware)
        self.assertIn("gpsParsedSentences", firmware)
        self.assertIn("GPS_SERVICE_INTERVAL_MS = 5", (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8"))
        self.assertIn("recorderMode != RECORDER_RECORDING", firmware)
        self.assertIn("benchMode != BENCH_OFF && recorderMode != RECORDER_RECORDING", firmware)
        self.assertIn("sensorReinitializationAllowed", firmware)
        self.assertIn("recorderMode != RECORDER_WATCHING", firmware)
        self.assertIn("resetCalibration(nowMs)", firmware)
        self.assertIn("RESULT_INCONCLUSIVE", bench)
        self.assertIn("peakGyroDps_ <= 20.0f", bench)
        self.assertNotIn("delay(", bench)

    def test_first_packet_radio_bench_contract(self):
        protocol = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonProtocol.h").read_text(encoding="utf-8")
        transmitter = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        receiver = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        self.assertIn("kBenchRadioMarker", protocol)
        self.assertIn("isBenchRadioPacket", protocol)
        self.assertIn('!strcmp(serialCommand, "radio test")', transmitter)
        self.assertIn("radioBenchPacketsRemaining = 5", transmitter)
        self.assertIn("packet.reserved = green_ion::kBenchRadioMarker", transmitter)
        self.assertIn("radio.setCADTimeout(0)", transmitter)
        self.assertIn("radio.mode() == RHGenericDriver::RHModeTx", transmitter)
        self.assertIn("green_ion::isBenchRadioPacket(packet)", receiver)
        self.assertIn('F("{\\\"type\\\":\\\"radio_bench\\\",\\\"protocol\\\":")', receiver)
        self.assertIn('F(",\\\"antenna_required\\\":true}")', receiver)

    def test_recorder_priority_telemetry_queue_contract(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        queue = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonTelemetryQueue.h").read_text(encoding="utf-8")
        self.assertIn("TELEMETRY_QUEUE_CAPACITY = 3", config)
        self.assertIn("RADIO_SERVICE_GUARD_US = 5000", config)
        self.assertIn("TelemetryQueue<config::TELEMETRY_QUEUE_CAPACITY>", firmware)
        self.assertIn("queueTelemetry(nowMs);", firmware)
        self.assertIn("serviceTelemetry(nowMs, micros());", firmware)
        self.assertLess(firmware.index("appendLog(nowUs"), firmware.index("serviceTelemetry(nowMs"))
        self.assertLess(firmware.index("recorderCore.flush()"), firmware.index("serviceTelemetry(nowMs"))
        self.assertIn("radio.mode() == RHGenericDriver::RHModeTx", firmware)
        self.assertIn("radio.setCADTimeout(0)", firmware)
        self.assertIn("radioDispatchAllowed", queue)
        self.assertIn("dropped_", queue)

    def test_radio_recovery_never_preempts_critical_recording(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        recovery = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonRadioRecovery.h").read_text(encoding="utf-8")
        self.assertIn("RADIO_SEND_FAILURE_THRESHOLD = 3", config)
        self.assertIn("RADIO_REINITIALIZE_DELAY_MS = 10000", config)
        self.assertIn("RadioRecovery radioRecovery", firmware)
        self.assertIn("serviceRadioRecovery(nowMs)", firmware)
        self.assertIn("radio retry", firmware)
        self.assertIn("recorderCritical", recovery)
        self.assertIn("consecutiveSendFailures_ < threshold_", recovery)
        self.assertIn("static_cast<int32_t>(nowMs - nextRetryMs_)", recovery)

    def test_sensor_log_is_separate_and_crc_protected(self):
        sensor_log = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonSensorLog.h").read_text(encoding="utf-8")
        rkt4 = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonRkt4.h").read_text(encoding="utf-8")
        self.assertIn('memcpy(out, "GIS1", 4)', sensor_log)
        self.assertIn("kHeaderBytes = 32", sensor_log)
        self.assertIn("kRecordBytes = 68", sensor_log)
        self.assertIn("rkt4HeaderCrc32", sensor_log)
        self.assertIn("timestampUs", sensor_log)
        self.assertIn("accelerationX", sensor_log)
        self.assertIn("gyroZ", sensor_log)
        self.assertIn("gpsFixAgeMs", sensor_log)
        self.assertIn("crc32(out, 64)", sensor_log)
        self.assertIn('memcpy(out, "RKT4", 4)', rkt4)
        self.assertIn("kVersion = 2", rkt4)

    def test_sensor_log_firmware_preserves_primary_recorder_priority(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        core = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonSensorLogCore.h").read_text(encoding="utf-8")
        self.assertIn("SENSOR_LOG_QUEUE_RECORDS = 16", config)
        self.assertIn("SENSOR_LOG_SERVICE_GUARD_US = 7000", config)
        self.assertIn('sensorLogFilename.replace(".BIN", ".SEN")', firmware)
        self.assertIn("SensorLogCore<config::PREBUFFER_RECORDS", firmware)
        self.assertIn("sensorLogCore.observe(makeSensorLogRecord(nowUs, nowMs))", firmware)
        self.assertIn("serviceSensorLog(nowMs, micros())", firmware)
        self.assertLess(
            firmware.index("recorderCore.observe(record)"),
            firmware.index("sensorLogCore.observe(makeSensorLogRecord"),
        )
        self.assertLess(
            firmware.index("recorderCore.stop()"),
            firmware.index("sensorLogCore.stop()"),
        )
        loop = firmware[firmware.index("void loop()") :]
        self.assertLess(loop.index("recorderCore.flush()"), loop.index("serviceSensorLog(nowMs"))
        self.assertLess(loop.index("serviceSensorLog(nowMs"), loop.index("serviceTelemetry(nowMs"))
        self.assertIn("Record records_[Capacity]", core)
        self.assertIn("SENSOR_CORE_WRITE_FAILED", core)
        self.assertIn("SENSOR_CORE_FLUSH_FAILED", core)

    def test_sensor_recovery_is_read_only_and_bound_to_rkt4(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        recovery = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonSensorRecovery.h").read_text(encoding="utf-8")
        self.assertGreaterEqual(firmware.count("appendSensorRecoveryEvidence("), 2)
        self.assertIn('sensorLeaf.replace(".BIN", ".SEN")', firmware)
        self.assertIn("sensor_binding_valid", firmware)
        self.assertIn("sensor_valid_prefix_records", firmware)
        self.assertIn("sensor_trailing_partial_bytes", firmware)
        self.assertIn("headerMatchesFlight", recovery)
        self.assertIn("sensor_log::crc32(rkt4Header", recovery)
        self.assertIn("sensor_log::decodeRecord", recovery)
        self.assertNotIn("SD.remove(sensorPath)", firmware)
        self.assertNotIn("sensor.truncate", firmware)

    def test_barometer_recovery_is_bounded_and_deferred_while_active(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        health = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonSensorHealth.h").read_text(encoding="utf-8")
        self.assertIn("BAROMETER_READ_FAILURE_THRESHOLD = 3", config)
        self.assertIn("BAROMETER_REINITIALIZE_DELAY_MS = 5000", config)
        self.assertIn("SensorHealth barometerHealth", firmware)
        self.assertIn("serviceBarometerRecovery(nowMs)", firmware)
        self.assertIn("recorderMode == RECORDER_WATCHING", firmware)
        self.assertIn("recorderMode == RECORDER_RECORDING", firmware)
        self.assertIn("HEALTH_BARO_READ_ERROR", firmware)
        self.assertIn('body += ",\\\"barometer_runtime_losses\\\":"', firmware)
        self.assertLess(firmware.index("serviceSensorLog(nowMs, micros())"),
                        firmware.index("serviceBarometerRecovery(nowMs)"))
        self.assertIn("static_cast<int32_t>(nowMs - nextRetryMs_)", health)

    def test_gic2_checkpoints_only_describe_flushed_sensor_bytes(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        checkpoint = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonCheckpoint.h").read_text(encoding="utf-8")
        metadata = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonMetadata.h").read_text(encoding="utf-8")
        self.assertIn('memcpy(out, "GIC1", 4)', checkpoint)
        self.assertIn('memcpy(out, "GIC2", 4)', checkpoint)
        self.assertIn("kCheckpointV1Bytes = 32", checkpoint)
        self.assertIn("kCheckpointBytes = 48", checkpoint)
        self.assertIn("sensorCommittedBytes", checkpoint)
        self.assertIn("sensorCheckpointForFile", checkpoint)
        self.assertIn("sensorLogCommittedBytes", firmware)
        self.assertIn("sensorLogCheckpointable", firmware)
        self.assertIn("sensorLogCore.flushIfDrained()", firmware)
        self.assertLess(
            firmware.index("sensorLogCore.flushIfDrained()"),
            firmware.index("sensorLogCommittedBytes = static_cast<uint32_t>(sensorLogFile.size())", firmware.index("sensorLogCore.flushIfDrained()")),
        )
        self.assertIn("checkpoint_format=GIC2", metadata)


if __name__ == "__main__":
    unittest.main()

