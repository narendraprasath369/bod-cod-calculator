"""
BOD and COD calculator for water quality testing.

BOD (5-day, dilution method, APHA 5210 B)
    Without seed:  BOD = (D1 - D2) / P
    With seed:     BOD = ((D1 - D2) - (B1 - B2) * f) / P
    D1 = initial DO of diluted sample (mg/L)
    D2 = DO after 5 days incubation at 20 C (mg/L)
    B1, B2 = initial / final DO of seed control (mg/L)
    P = sample volume / bottle volume (decimal fraction)
    f = (% seed in sample) / (% seed in control)

COD (closed/open reflux, dichromate, APHA 5220)
    COD (mg O2/L) = (A - B) * M * 8000 / V
    A = mL FAS used for blank
    B = mL FAS used for sample
    M = molarity of FAS
    V = sample volume (mL)
"""


def calculate_bod(d1, d2, sample_ml, bottle_ml=300.0,
                  b1=None, b2=None, seed_factor=1.0):
    """Return BOD5 in mg/L."""
    if sample_ml <= 0 or bottle_ml <= 0 or sample_ml > bottle_ml:
        raise ValueError("Sample volume must be > 0 and <= bottle volume.")
    p = sample_ml / bottle_ml
    depletion = d1 - d2
    if depletion < 2.0:
        print("Warning: DO depletion < 2 mg/L. Use a larger sample volume.")
    if d2 < 1.0:
        print("Warning: residual DO < 1 mg/L. Use a smaller sample volume.")
    seed_correction = 0.0
    if b1 is not None and b2 is not None:
        seed_correction = (b1 - b2) * seed_factor
    return (depletion - seed_correction) / p


def calculate_cod(blank_ml, sample_titre_ml, fas_molarity, sample_ml):
    """Return COD in mg O2/L."""
    if sample_ml <= 0:
        raise ValueError("Sample volume must be > 0.")
    if blank_ml < sample_titre_ml:
        print("Warning: blank titre is smaller than sample titre. Check readings.")
    return (blank_ml - sample_titre_ml) * fas_molarity * 8000 / sample_ml


def fas_molarity(k2cr2o7_ml, fas_ml, k2cr2o7_molarity=0.04167):
    """Standardise FAS: M(FAS) = vol K2Cr2O7 * 0.25 N / vol FAS (default 0.25 N = 0.04167 M)."""
    normality = k2cr2o7_molarity * 6
    return k2cr2o7_ml * normality / fas_ml


def classify(bod, cod):
    ratio = bod / cod if cod else 0
    if ratio > 0.5:
        note = "Easily biodegradable (BOD/COD > 0.5)"
    elif ratio >= 0.3:
        note = "Moderately biodegradable (BOD/COD 0.3-0.5)"
    else:
        note = "Poorly biodegradable / possible toxic or inorganic load (< 0.3)"
    return ratio, note


def ask(prompt, cast=float, default=None):
    while True:
        raw = input(f"{prompt}" + (f" [{default}]" if default is not None else "") + ": ").strip()
        if not raw and default is not None:
            return default
        try:
            return cast(raw)
        except ValueError:
            print("Invalid input, try again.")


def main():
    print("=== Water Test Calculator: BOD & COD ===")
    bod = cod = None

    if input("\nCalculate BOD? (y/n) [y]: ").strip().lower() != "n":
        d1 = ask("Initial DO, D1 (mg/L)")
        d2 = ask("DO after 5 days, D2 (mg/L)")
        sample = ask("Sample volume (mL)")
        bottle = ask("BOD bottle volume (mL)", default=300.0)
        b1 = b2 = None
        f = 1.0
        if input("Use seed correction? (y/n) [n]: ").strip().lower() == "y":
            b1 = ask("Seed control initial DO, B1 (mg/L)")
            b2 = ask("Seed control final DO, B2 (mg/L)")
            f = ask("Seed factor f", default=1.0)
        bod = calculate_bod(d1, d2, sample, bottle, b1, b2, f)
        print(f"\nBOD5 = {bod:.2f} mg/L")

    if input("\nCalculate COD? (y/n) [y]: ").strip().lower() != "n":
        blank = ask("Blank titre, A (mL FAS)")
        titre = ask("Sample titre, B (mL FAS)")
        molarity = ask("FAS molarity, M")
        vol = ask("Sample volume (mL)", default=20.0)
        cod = calculate_cod(blank, titre, molarity, vol)
        print(f"\nCOD = {cod:.2f} mg O2/L")

    if bod is not None and cod:
        ratio, note = classify(bod, cod)
        print(f"\nBOD/COD ratio = {ratio:.2f} -> {note}")


if __name__ == "__main__":
    main()
