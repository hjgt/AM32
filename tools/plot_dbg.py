#!/usr/bin/env python3
# plot_dbg.py - offline analysis of the ZCD diagnostic capture
# ------------------------------------------------------------
# Reads the CSV produced by tools/dump_dbg.py and:
#   1) plots bemf vs neutral (and the raw va/vb/vc) over the capture,
#   2) marks the 'out' polarity flips (software zero-cross decisions),
#   3) prints simple diagnostics that help decide whether the low-speed
#      problem is a SYSTEMATIC neutral offset or RANDOM jitter / low amplitude.
#
# Usage:
#   python3 tools/plot_dbg.py dbg_capture.csv
#   python3 tools/plot_dbg.py dbg_capture.csv --no-show --save fig.png
#
# Interpretation guide (see porting notes 12.21):
#   - margin = bemf - neutral. At a clean zero-cross it should swing
#     symmetrically through 0 with the commutation. If 'margin' is small
#     relative to its own swing (low SNR) -> amplitude/noise problem.
#   - If 'out' flips do NOT line up with margin sign changes, or margin has
#     a persistent DC bias (mean far from 0 while the phase is floating) ->
#     systematic offset problem.
#   - flips_per_row very high with tiny margin -> chattering / random jitter.

import sys
import csv
import argparse


def load(path):
    rows = []
    with open(path, newline="") as f:
        r = csv.DictReader(f)
        for d in r:
            rows.append({k: int(v) for k, v in d.items()})
    return rows


def diagnostics(rows):
    n = len(rows)
    if n == 0:
        print("empty capture")
        return
    margins = [r["bemf"] - r["neutral"] for r in rows]
    mean_m = sum(margins) / n
    amp = max(margins) - min(margins)
    # count out flips
    flips = sum(1 for i in range(1, n) if rows[i]["out"] != rows[i - 1]["out"])
    # count margin sign changes
    sign_changes = 0
    for i in range(1, n):
        if (margins[i] > 0) != (margins[i - 1] > 0):
            sign_changes += 1
    zc_span = rows[-1]["zero_crosses"] - rows[0]["zero_crosses"]
    ci_vals = [r["ci"] for r in rows if r["ci"] > 0]
    ci_mean = (sum(ci_vals) / len(ci_vals)) if ci_vals else 0

    print("=== capture diagnostics ===")
    print("rows                : %d" % n)
    print("zero_crosses span   : %d (start=%d end=%d)"
          % (zc_span, rows[0]["zero_crosses"], rows[-1]["zero_crosses"]))
    print("commutation_interval: mean=%.1f (0.5us units)" % ci_mean)
    print("margin (bemf-neutral): mean=%.1f  min=%d  max=%d  peak-peak=%d"
          % (mean_m, min(margins), max(margins), amp))
    print("out polarity flips  : %d" % flips)
    print("margin sign changes : %d" % sign_changes)
    if amp > 0:
        print("mean/amp ratio      : %.3f  (large magnitude => systematic DC bias)"
              % (abs(mean_m) / amp))
    print()
    print("HINT:")
    print(" - |mean|/amp small AND flips ~ margin-sign-changes => clean ZCD.")
    print(" - |mean|/amp large (persistent offset)            => systematic offset.")
    print(" - small peak-peak amp AND many flips              => low SNR / jitter.")


def plot(rows, show, save):
    try:
        import matplotlib
        if not show:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed; skipping plot. "
              "Install with: pip install matplotlib")
        return

    x = [r["i"] for r in rows]
    va = [r["va"] for r in rows]
    vb = [r["vb"] for r in rows]
    vc = [r["vc"] for r in rows]
    neutral = [r["neutral"] for r in rows]
    bemf = [r["bemf"] for r in rows]
    margin = [r["bemf"] - r["neutral"] for r in rows]
    out = [r["out"] for r in rows]

    fig, ax = plt.subplots(3, 1, figsize=(12, 9), sharex=True)

    ax[0].plot(x, va, label="va", alpha=0.5)
    ax[0].plot(x, vb, label="vb", alpha=0.5)
    ax[0].plot(x, vc, label="vc", alpha=0.5)
    ax[0].plot(x, neutral, "k--", label="neutral", linewidth=1.2)
    ax[0].set_ylabel("ADC counts")
    ax[0].set_title("Raw phase dividers and reconstructed neutral")
    ax[0].legend(loc="upper right", ncol=4, fontsize=8)

    ax[1].plot(x, bemf, label="bemf (floating)", color="tab:blue")
    ax[1].plot(x, neutral, "k--", label="neutral", linewidth=1.0)
    ax[1].set_ylabel("ADC counts")
    ax[1].set_title("Floating-phase BEMF vs neutral")
    ax[1].legend(loc="upper right", fontsize=8)

    ax[2].plot(x, margin, label="margin = bemf - neutral", color="tab:green")
    ax[2].axhline(0, color="r", linewidth=0.8)
    # overlay out flips as vertical markers
    for i in range(1, len(rows)):
        if out[i] != out[i - 1]:
            ax[2].axvline(x[i], color="tab:orange", alpha=0.3, linewidth=0.8)
    ax[2].set_ylabel("counts")
    ax[2].set_xlabel("sample index")
    ax[2].set_title("Zero-cross margin (orange = out flip)")
    ax[2].legend(loc="upper right", fontsize=8)

    fig.tight_layout()
    if save:
        fig.savefig(save, dpi=110)
        print("saved figure to %s" % save)
    if show:
        plt.show()


def main():
    p = argparse.ArgumentParser(description="Analyze ZCD dbg_buf capture")
    p.add_argument("csv", help="CSV produced by dump_dbg.py")
    p.add_argument("--no-show", dest="show", action="store_false",
                   help="do not open an interactive window")
    p.add_argument("--save", default=None, help="save figure to PNG path")
    args = p.parse_args()

    rows = load(args.csv)
    diagnostics(rows)
    plot(rows, args.show, args.save)


if __name__ == "__main__":
    main()
