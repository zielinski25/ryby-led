package pl.aquarium.lednative.core

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class ModelsTest {

    /** Ksztalt z Firebase RTDB: mode jako tekst, temps[] zamiast temp1/temp2/water. */
    @Test fun statusZChmury() {
        val json = JSONObject(
            """
            {
              "power": true, "mode": "AUTO", "pumpOn": false,
              "pwm": [100, 200, 300, 400, 500],
              "temps": [31.5, 33.2, 25.4],
              "luxRoom": 800.5, "luxNadWoda": 1500.0,
              "energyTodayWh": 1234.0, "kwhPrice": 1.1,
              "updatedAt": 987654,
              "fwVersion": "4.8.4",
              "schedule": {"morningWD": 400, "morningWE": 450, "middayOff": 800,
                           "eveningBefore": -30, "sunsetMin": 1180}
            }
            """.trimIndent()
        )
        val s = DeviceStatus.parse(json)
        assertTrue(s.power)
        assertTrue(s.auto)
        assertFalse(s.pumpOn)
        assertEquals(listOf(100, 200, 300, 400, 500), s.pwm)
        assertEquals(31.5, s.tempPlate1!!, 0.001)
        assertEquals(25.4, s.tempWater!!, 0.001)
        assertEquals(400, s.schedule.morningWD)
        assertEquals(-30, s.schedule.eveningBefore)
        assertEquals(1180, s.schedule.sunsetMin)
        assertEquals(987654L, s.updatedAt)
        assertEquals(300, s.pwmAverage)
        assertTrue(s.hasEnergy)
    }

    /** Ksztalt z LAN /api/status: tryb/mode jako boolean, temp1/temp2/water. */
    @Test fun statusZLan() {
        val json = JSONObject(
            """
            {
              "power": false, "tryb": false, "mode": false,
              "pwm": [0,0,0,0,0],
              "temp1": 30.0, "temp2": 31.0, "water": 24.8,
              "luxPokojowy": 120.0, "luxNadWoda": 300.0,
              "fade": 45, "sensInt": 60, "rampSec": 20,
              "pumpSlots": [{"start": 480, "end": 600}, {"start": 1080, "end": 1140}],
              "fsUsed": 2048, "fsTotal": 65536
            }
            """.trimIndent()
        )
        val s = DeviceStatus.parse(json)
        assertFalse(s.power)
        assertFalse(s.auto)
        assertEquals(24.8, s.tempWater!!, 0.001)
        assertEquals(45, s.fadeMinutes)
        assertEquals(2, s.pumpSlots.size)
        assertEquals("08:00", s.pumpSlots[0].start)
        assertEquals("10:00", s.pumpSlots[0].end)
        assertEquals(65536L, s.fsTotal)
        assertNull(s.updatedAt)
        assertFalse(s.hasEnergy)
    }

    @Test fun pumpSlotsPrzyjmujaObaKsztalty() {
        val a = DeviceStatus.parse(JSONObject("""{"pumpSlots":[{"s":"07:15","e":"09:45"}]}"""))
        assertEquals("07:15", a.pumpSlots.single().start)
        val b = DeviceStatus.parse(JSONObject("""{"pumpSlots":[{"start":435,"end":585}]}"""))
        assertEquals("07:15", b.pumpSlots.single().start)
        assertEquals("09:45", b.pumpSlots.single().end)
    }

    @Test fun pustyJsonNieWywracaAplikacji() {
        val s = DeviceStatus.parse(JSONObject("{}"))
        assertFalse(s.power)
        assertTrue(s.auto)
        assertEquals(List(5) { 0 }, s.pwm)
        assertNull(s.tempWater)
        assertNull(s.energyTodayWh)
        assertEquals(0, s.pwmPercent)
    }

    @Test fun nullWPolachNieTworzyNaN() {
        val s = DeviceStatus.parse(
            JSONObject("""{"temp1":null,"water":null,"energyTodayWh":null,"pwm":[null,10]}""")
        )
        assertNull(s.tempPlate1)
        assertNull(s.tempWater)
        assertNull(s.energyTodayWh)
        assertEquals(listOf(0, 10, 0, 0, 0), s.pwm)
    }

    @Test fun pwmZawszeMaPiecElementow() {
        assertEquals(5, DeviceStatus.parse(JSONObject("""{"pwm":[1,2]}""")).pwm.size)
        assertEquals(5, DeviceStatus.parse(JSONObject("""{"pwm":[1,2,3,4,5,6,7]}""")).pwm.size)
    }

    @Test fun parsowanieLogu() {
        val e = LogEntry.parse("12:34:56 lvl=ERROR tag=POMPA msg=\"przekroczenie\"")
        assertEquals("12:34:56", e.time)
        assertEquals("ERROR", e.level)
        assertEquals("err", e.category)
        val w = LogEntry.parse("08:00:00 lvl=WARN tag=EMA alpha=0.25")
        assertEquals("warn", w.category)
        val all = LogEntry.parseAll("linia1\n\nlinia2\r\n")
        assertEquals(2, all.size)
    }

    @Test fun listaPlikow() {
        val fs = FsListing.parse(
            """{"files":[{"name":"/log.txt","size":80000},{"name":"/cfg.json","size":120}],"used":80120,"total":262144}"""
        )
        assertEquals(2, fs.files.size)
        assertEquals("/log.txt", fs.files[0].name)
        assertEquals(80000L, fs.files[0].size)
        assertEquals(262144L, fs.total)
    }

    @Test fun settingsZeStatusu() {
        val s = DeviceStatus.parse(
            JSONObject("""{"sensorMode":1,"adaptEnabled":true,"minLuxTarget":3000,"emaFilter":0.3}""")
        )
        val cfg = DeviceSettings.from(s)
        assertEquals(1, cfg.sensorMode)
        assertTrue(cfg.adaptEnabled)
        assertEquals(3000, cfg.minLuxTarget)
        assertEquals(0.3, cfg.emaFilter, 0.0001)
    }
}
