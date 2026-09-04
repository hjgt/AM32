# dump_dbg.py - GDB embedded Python script
# ----------------------------------------
# Dumps the comparator.c ZCD diagnostic ring buffer (dbg_buf) out over SWD
# into a CSV file for offline analysis.
#
# Usage (inside an active GDB session attached to the target, motor stopped):
#   (gdb) source tools/dump_dbg.py
#   (gdb) dumpdbg dbg_capture.csv
#
# Notes:
#   - Reads dbg_idx to know how many valid entries exist (buffer stops when
#     full at DBG_BUF_SIZE=512).
#   - Reads each struct dbg_entry field by name via GDB, so it is robust to
#     struct padding and target endianness (no raw memory byte parsing).
#   - Run with the motor STOPPED (after your test) to avoid reading a buffer
#     that is being written concurrently.

import gdb
import csv

DBG_BUF_SIZE = 512

FIELDS = ["i", "va", "vb", "vc", "neutral", "bemf",
          "zero_crosses", "ci", "step", "running", "out", "phase"]


class DumpDbg(gdb.Command):
    """dumpdbg [outfile.csv] - dump dbg_buf to CSV"""

    def __init__(self):
        super(DumpDbg, self).__init__("dumpdbg", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        outfile = arg.strip() or "dbg_capture.csv"

        try:
            idx = int(gdb.parse_and_eval("dbg_idx"))
        except gdb.error as e:
            print("ERROR: cannot read dbg_idx (is the symbol in scope?): %s" % e)
            return

        n = idx if idx < DBG_BUF_SIZE else DBG_BUF_SIZE
        if n == 0:
            print("dbg_idx == 0: buffer is empty. Did the capture run?")
            return

        with open(outfile, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(FIELDS)
            for i in range(n):
                e = gdb.parse_and_eval("dbg_buf[%d]" % i)
                row = [i]
                for fld in FIELDS[1:]:
                    row.append(int(e[fld]))
                w.writerow(row)

        print("wrote %d rows to %s (dbg_idx=%d)" % (n, outfile, idx))


DumpDbg()
print("dump_dbg.py loaded. Run: dumpdbg dbg_capture.csv")
