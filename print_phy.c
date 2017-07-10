/* This file is part of phytool
 *
 * phytool is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * phytool is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with phytool.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>

#include <linux/mdio.h>
#include <net/if.h>

#include "phytool.h"

void print_bool(const char *name, int on)
{
	if (on)
		fputs("\e[1m+", stdout);
	else
		fputs("-", stdout);

	fputs(name, stdout);

	if (on)
		fputs("\e[0m", stdout);
}

static void print_hex(const char *name, int indent, uint16_t val){
	printf("%*s%-*s0x%04x\n", indent + INDENT, "", 24, name, val);
}

static void _print_choice(const char *name, int indent, uint16_t val, const char **choices, int len){
	val = val % len;
	printf("%*s%-*s%s\n", indent + INDENT, "", 24, name, choices[val]);
}
#define print_choice(name, indent, val, choices) _print_choice(name, indent, val, choices, sizeof(choices) / sizeof(choices[0]))

void print_attr_name(const char *name, int indent)
{
	int start, end, len;

	printf("%*s%n%s:%n", indent, "", &start, name, &end);

	len = end - start;
	printf("%*s", (len > 16) ? 0 : 16 - len, "");
}

static void ieee_bmcr(uint16_t val, int indent)
{
	int speed = 10;

	if (val & BMCR_SPEED100)
		speed = 100;
	if (val & BMCR_SPEED1000)
		speed = 1000;

	printf("%*sieee-phy: reg: Basic Control(0x00) val:%#.4x\n", indent, "", val);

	print_attr_name("flags", indent + INDENT);
	print_bool("reset", val & BMCR_RESET);
	putchar(' ');

	print_bool("loopback", val & BMCR_LOOPBACK);
	putchar(' ');

	print_bool("aneg-enable", val & BMCR_ANENABLE);
	putchar(' ');

	print_bool("power-down", val & BMCR_PDOWN);
	putchar(' ');

	print_bool("isolate", val & BMCR_ISOLATE);
	putchar(' ');

	print_bool("aneg-restart", val & BMCR_ANRESTART);
	putchar(' ');

	print_bool("collision-test", val & BMCR_CTST);
	putchar('\n');

	print_attr_name("speed", indent + INDENT);
	printf("%d-%s\n", speed, (val & BMCR_FULLDPLX) ? "full" : "half");
}

static void ieee_bmsr(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Basic Status (0x01) val:%#.4x\n", indent, "", val);

	print_attr_name("capabilities", indent + INDENT);
	print_bool("100-b4", val & BMSR_100BASE4);
	putchar(' ');

	print_bool("100-f", val & BMSR_100FULL);
	putchar(' ');

	print_bool("100-h", val & BMSR_100HALF);
	putchar(' ');

	print_bool("10-f", val & BMSR_10FULL);
	putchar(' ');

	print_bool("10-h", val & BMSR_10HALF);
	putchar(' ');

	print_bool("100-t2-f", val & BMSR_100FULL2);
	putchar(' ');

	print_bool("100-t2-h", val & BMSR_100HALF2);
	putchar('\n');

	print_attr_name("flags", indent + INDENT);
	print_bool("ext-status", val & BMSR_ESTATEN);
	putchar(' ');

	print_bool("aneg-complete", val & BMSR_ANEGCOMPLETE);
	putchar(' ');

	print_bool("remote-fault", val & BMSR_RFAULT);
	putchar(' ');

	print_bool("aneg-capable", val & BMSR_ANEGCAPABLE);
	putchar(' ');

	print_bool("link", val & BMSR_LSTATUS);
	putchar(' ');

	print_bool("jabber", val & BMSR_JCD);
	putchar(' ');

	print_bool("ext-register", val & BMSR_ERCAP);
	putchar('\n');
}

static void ieee_phy_id1(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: PHY ID Register 1 (0x02) val:%#.4x\n", indent, "", val);

	print_hex("OUI", indent, val);
}

static void ieee_phy_id2(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: PHY ID Register 2 (0x03) val:%#.4x\n", indent, "", val);

	print_hex("OUI", indent, (val & 0xfc00) >> 10);
	print_hex("Model Number", indent, (val & 0x03f0) >> 4);
	print_hex("Revision Number", indent, (val & 0x000f));
}

static void ieee_ana(uint16_t val, int indent)
{
	static const char *pause_choice[4] = {
		"NO PAUSE",
		"Assymetric PAUSE (link partner)",
		"Symmetric PAUSE",
		"Symmetric & assymetric PAUSE (local device)"
	};

	printf("%*sieee-phy: reg: Autonegotiation Advertisement (0x04) val:%#.4x\n", indent, "", val);

	print_attr_name("capabilities", indent + INDENT);
	print_bool("next-page", val & (1 << 15)); putchar(' ');
	print_bool("remote-fault-support", val & (1 << 13)); putchar(' ');
	print_bool("t4-capable", val & (1 << 9)); putchar(' ');
	print_bool("100M-FD-capable", val & (1 << 8)); putchar(' ');
	print_bool("100M-HD-capable", val & (1 << 7)); putchar(' ');
	print_bool("10M-FD-capable", val & (1 << 6)); putchar(' ');
	print_bool("10M-HD-capable", val & (1 << 5)); putchar(' ');
	putchar('\n');

	print_choice("Pause frame support", indent, (val >> 10) & 0x3, pause_choice);
	print_hex("Sector (1 == IEEE802.3)", indent, val & 0xf);
}

static void ieee_anlpa(uint16_t val, int indent)
{
	static const char *pause_choice[4] = {
		"NO PAUSE",
		"Assymetric PAUSE (link partner)",
		"Symmetric PAUSE",
		"Symmetric & assymetric PAUSE (local device)"
	};

	printf("%*sieee-phy: reg: Autonegotiation Link Partner Ability (0x05) val:%#.4x\n", indent, "", val);

	print_attr_name("capabilities", indent + INDENT);
	print_bool("next-page", val & (1 << 15)); putchar(' ');
	print_bool("link-code-word-received", val & (1 << 14)); putchar(' ');
	print_bool("remote-fault-detected", val & (1 << 13)); putchar(' ');
	print_bool("t4-capable", val & (1 << 9)); putchar(' ');
	print_bool("100M-FD-capable", val & (1 << 8)); putchar(' ');
	print_bool("100M-HD-capable", val & (1 << 7)); putchar(' ');
	print_bool("10M-FD-capable", val & (1 << 6)); putchar(' ');
	print_bool("10M-HD-capable", val & (1 << 5)); putchar(' ');
	putchar('\n');

	print_choice("Pause frame support", indent, (val >> 10) & 0x3, pause_choice);
	print_hex("Sector (1 == IEEE802.3)", indent, val & 0xf);
}

static void ieee_anexp(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Autonegotiation Expansion (0x06) val:%#.4x\n", indent, "", val);

	print_attr_name("capabilities", indent + INDENT);
	print_bool("parallel-detection-fault", val & (1 << 4)); putchar(' ');
	print_bool("link-partner-has-next-page", val & (1 << 3)); putchar(' ');
	print_bool("local-device-has-next-page", val & (1 << 2)); putchar(' ');
	print_bool("page-received", val & (1 << 1)); putchar(' ');
	print_bool("link-partner-autoneg-enabled", val & (1 << 1)); putchar(' ');
	putchar('\n');
}

static void ieee_annp(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Autonegotiation Next Page (0x07) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_anlpnp(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Autonegotiation Link Partner Next Page Ability (0x08) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_1gctrl(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: 1000Base-T Control (0x09) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_1gsts(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: 1000Base-T Status (10, 0x0a) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_rextctrl(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Extended Register Control (11, 0x0b) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_rextwr(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Extended Register Write (12, 0x0c) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_rextrd(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Extended Register Read (13, 0x0d) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_extmiists(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Extended MII Status (15, 0x0f) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_loopledmode(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Remote Loopback, LED Mode (17, 0x11) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_pmasts(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Digital PMA/PCS Status (19, 0x13) val:%#.4x\n", indent, "", val);
	print_attr_name("status", indent + INDENT);
	print_bool("1000baset-ok", val & (1 << 2)); putchar(' ');
	print_bool("100baset-ok", val & (1 << 1)); putchar(' ');
}

static void ieee_rxerr(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: RX Error Counter (21, 0x15) val:%#.4x\n", indent, "", val);
}

static void ieee_intctrl(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Interrupt Control (27, 0x1b) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_dbgctrl(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: Digital Debug Control (28, 0x1c) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}

static void ieee_phyctrl(uint16_t val, int indent)
{
	printf("%*sieee-phy: reg: PHY Control (31, 0x1f) val:%#.4x\n", indent, "", val);
	// TODO: print bits
}


static void (*ieee_reg_printers[32])(uint16_t, int) = {
	[0] = ieee_bmcr,
	[1] = ieee_bmsr,
	[2] = ieee_phy_id1,
	[3] = ieee_phy_id2,
	[4] = ieee_ana,
	[5] = ieee_anlpa,
	[6] = ieee_anexp,
	[7] = ieee_annp,
	[8] = ieee_anlpnp,
	[9] = ieee_1gctrl,
	[10] = ieee_1gsts,
	[11] = ieee_rextctrl,
	[12] = ieee_rextwr,
	[13] = ieee_rextrd,
	[15] = ieee_extmiists,
	[17] = ieee_loopledmode,
	[19] = ieee_pmasts,
	[21] = ieee_rxerr,
	[27] = ieee_intctrl,
	[28] = ieee_dbgctrl,
	[31] = ieee_phyctrl,
};

static int ieee_one(const struct loc *loc, int indent)
{
	int val = phy_read(loc);

	if (val < 0)
		return val;

	if (loc->reg > 0x1f || !ieee_reg_printers[loc->reg])
		printf("%*sieee-phy: reg:%#.2x val:%#.4x\n", indent, "",
		       loc->reg, val);
	else
		ieee_reg_printers[loc->reg](val, indent);

	return 0;
}

int print_phy_ieee(const struct loc *loc, int indent)
{
	struct loc loc_sum = *loc;
	int c = 0;

	if (loc->reg != REG_SUMMARY)
		return ieee_one(loc, indent);

	printf("%*sieee-phy: id:%#.8x\n\n", indent, "", phy_id(loc));

	for(c = 0; c < 0x1f; c++){
		loc_sum.reg = c;
		ieee_one(&loc_sum, indent + INDENT);

		putchar('\n');
	}
	return 0;
}

struct printer {
	uint32_t id;
	uint32_t mask;

	int (*print)(const struct loc *loc, int indent);
};

struct printer printer[] = {
	{ .id = 0, .mask = 0, .print = print_phy_ieee },
	{ .print = NULL }
};

int print_phytool(const struct loc *loc, int indent)
{
	struct printer *p;
	uint32_t id = phy_id(loc);

	for (p = printer; p->print; p++)
		if ((id & p->mask) == p->id)
			return p->print(loc, indent);

	return -1;
}
