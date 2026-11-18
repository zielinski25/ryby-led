package pl.aquarium.lednative.core

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test

class LogicTest {

    @Test fun pwmCommandMaPiecKanalow() {
        assertEquals("pwm 100 200 300 400 500", Logic.pwmCommand(listOf(100, 200, 300, 400, 500)))
    }

    @Test fun pwmPrzycinaIZerujeBraki() {
        assertEquals("pwm 1023 0 0 0 0", Logic.pwmCommand(listOf(5000, -20)))
        assertEquals(0, Logic.clampPwm(null))
        assertEquals(1023, Logic.clampPwm(99999))
        assertEquals(0, Logic.clampPwm(-1))
    }

    @Test fun tsZawszeRosnie() {
        assertEquals(1000L, Logic.nextTs(1000L, 0L))
        assertEquals(1001L, Logic.nextTs(500L, 1000L))
        assertEquals(2000L, Logic.nextTs(2000L, 1999L))
    }

    @Test fun adresBazyOdrzucaHttpIObceDomeny() {
        val ok = "https://akwarium-367be-default-rtdb.europe-west1.firebasedatabase.app"
        assertEquals(ok, Logic.validateDbUrl("$ok/"))
        listOf(
            "http://akwarium.firebasedatabase.app",
            "https://evil.example.com",
            "https://x.firebasedatabase.app?auth=abc",
            "https://user:pass@x.firebasedatabase.app",
            ""
        ).forEach { bad ->
            try {
                Logic.validateDbUrl(bad)
                fail("Powinno odrzucic: $bad")
            } catch (_: IllegalArgumentException) {
                // oczekiwane
            }
        }
    }

    @Test fun godzinyKonwersja() {
        assertEquals(390, Logic.toMinutes("06:30"))
        assertEquals("06:30", Logic.fmtHHMM(390))
        assertEquals("00:00", Logic.fmtHHMM(1440))
        assertEquals("23:45", Logic.fmtHHMM(-15))
        assertEquals("--:--", Logic.fmtHHMM(null))
    }

    @Test fun hhmmWalidacja() {
        assertTrue(Logic.HHMM.matches("00:00"))
        assertTrue(Logic.HHMM.matches("23:59"))
        assertFalse(Logic.HHMM.matches("24:00"))
        assertFalse(Logic.HHMM.matches("7:05"))
        assertFalse(Logic.HHMM.matches("12:60"))
    }

    @Test fun oknoPrzezPolnoc() {
        assertEquals(120, Logic.durationBetween("23:00", "01:00"))
        assertEquals(120, Logic.durationBetween("08:00", "10:00"))
        assertEquals(0, Logic.durationBetween("bad", "10:00"))
    }

    @Test fun zachodZOffsetem() {
        assertEquals("19:15", Logic.shiftedTime(1200, -45))
        assertEquals("--:--", Logic.shiftedTime(null, -45))
    }

    @Test fun stanPolaczeniaWymagaObserwacjiZmiany() {
        val t0 = Logic.trackUpdate(null, 1000L, 0L)
        // Pojedynczy odczyt nie dowodzi, ze ESP zyje.
        assertEquals(Logic.Conn.UNVERIFIED, Logic.connectionState(t0, 10_000L))
        assertEquals(Logic.Conn.STALE, Logic.connectionState(t0, Logic.STALE_MS + 1))

        val t1 = Logic.trackUpdate(t0, 2000L, 20_000L)
        assertEquals(Logic.Conn.ONLINE, Logic.connectionState(t1, 25_000L))
        assertEquals(Logic.Conn.STALE, Logic.connectionState(t1, 20_000L + Logic.STALE_MS + 1))

        // Brak zmiany updatedAt nie odswieza "zywości".
        val t2 = Logic.trackUpdate(t1, 2000L, 30_000L)
        assertEquals(t1.changedAt, t2.changedAt)
        assertEquals(Logic.Conn.NONE, Logic.connectionState(null, 0L))
    }

    @Test fun formatyOdporneNaBraki() {
        assertEquals("—", Logic.fmtTemp(null))
        assertEquals("—", Logic.fmtTemp(-127.0))
        assertEquals("24.5", Logic.fmtTemp(24.5))
        assertEquals("—", Logic.fmtLux(null))
        assertEquals("1234", Logic.fmtLux(1234.4))
        assertEquals("—", Logic.fmtWh(null))
        assertEquals("500 Wh", Logic.fmtWh(500.0))
        assertEquals("1.50 kWh", Logic.fmtWh(1500.0))
        assertEquals("1.20", Logic.costPln(1000.0, 1.2))
        assertEquals("—", Logic.costPln(null, 1.2))
        assertEquals("2 h 05 min", Logic.fmtMinutes(125))
        assertEquals("45 min", Logic.fmtMinutes(45))
        assertEquals("1 d 2 h", Logic.fmtUptime(93_600L))
    }

    @Test fun kategorieLogow() {
        assertEquals("err", Logic.logCategory("ERROR", "", "cokolwiek"))
        assertEquals("warn", Logic.logCategory("WARN", "", "cokolwiek"))
        assertEquals("pump", Logic.logCategory("INFO", "POMPA", "start"))
        assertEquals("led", Logic.logCategory("INFO", "", "pwm=100"))
        assertEquals("all", Logic.logCategory("INFO", "", "hello"))
    }

    @Test fun znaneKomendy() {
        assertEquals("power_on", Logic.checkPlainCommand("power_on"))
        try {
            Logic.checkPlainCommand("drop_table")
            fail("Nieznana komenda powinna zostac odrzucona")
        } catch (_: IllegalArgumentException) {
        }
    }
}
