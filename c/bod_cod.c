/*
 * BOD and COD calculator for water quality testing (C version)
 *
 * BOD (5-day, dilution method, APHA 5210 B)
 *   Without seed: BOD = (D1 - D2) / P
 *   With seed:    BOD = ((D1 - D2) - (B1 - B2) * f) / P
 *   P = sample volume / bottle volume
 *
 * COD (dichromate reflux, APHA 5220)
 *   COD (mg O2/L) = (A - B) * M * 8000 / V
 *
 * Build: gcc -Wall -o bod_cod bod_cod.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Returns 0 on success, -1 on invalid volumes. Result in *out. */
int calculate_bod(double d1, double d2, double sample_ml, double bottle_ml,
                  int use_seed, double b1, double b2, double seed_factor,
                  double *out)
{
    if (sample_ml <= 0 || bottle_ml <= 0 || sample_ml > bottle_ml) {
        fprintf(stderr, "Error: sample volume must be > 0 and <= bottle volume.\n");
        return -1;
    }
    double p = sample_ml / bottle_ml;
    double depletion = d1 - d2;
    if (depletion < 2.0)
        printf("Warning: DO depletion < 2 mg/L. Use a larger sample volume.\n");
    if (d2 < 1.0)
        printf("Warning: residual DO < 1 mg/L. Use a smaller sample volume.\n");
    double seed_corr = use_seed ? (b1 - b2) * seed_factor : 0.0;
    *out = (depletion - seed_corr) / p;
    return 0;
}

int calculate_cod(double blank_ml, double sample_titre_ml, double fas_molarity,
                  double sample_ml, double *out)
{
    if (sample_ml <= 0) {
        fprintf(stderr, "Error: sample volume must be > 0.\n");
        return -1;
    }
    if (blank_ml < sample_titre_ml)
        printf("Warning: blank titre is smaller than sample titre. Check readings.\n");
    *out = (blank_ml - sample_titre_ml) * fas_molarity * 8000.0 / sample_ml;
    return 0;
}

/* Standardise FAS: M = vol K2Cr2O7 * normality / vol FAS (0.04167 M x 6 = 0.25 N) */
double fas_molarity(double k2cr2o7_ml, double fas_ml, double k2cr2o7_molarity)
{
    return k2cr2o7_ml * (k2cr2o7_molarity * 6.0) / fas_ml;
}

const char *classify(double bod, double cod, double *ratio)
{
    *ratio = (cod != 0.0) ? bod / cod : 0.0;
    if (*ratio > 0.5)   return "Easily biodegradable (BOD/COD > 0.5)";
    if (*ratio >= 0.3)  return "Moderately biodegradable (BOD/COD 0.3-0.5)";
    return "Poorly biodegradable / possible toxic or inorganic load (< 0.3)";
}

/* Prompt for a double. If has_default and input is empty, return default. */
double ask(const char *prompt, int has_default, double def)
{
    char buf[128], *end;
    for (;;) {
        if (has_default) printf("%s [%g]: ", prompt, def);
        else             printf("%s: ", prompt);
        if (!fgets(buf, sizeof buf, stdin)) exit(1);
        buf[strcspn(buf, "\r\n")] = '\0';
        if (buf[0] == '\0' && has_default) return def;
        double v = strtod(buf, &end);
        if (end != buf && *end == '\0') return v;
        printf("Invalid input, try again.\n");
    }
}

/* Yes/no prompt; returns 1 for yes. */
int ask_yn(const char *prompt, int default_yes)
{
    char buf[32];
    printf("%s (y/n) [%c]: ", prompt, default_yes ? 'y' : 'n');
    if (!fgets(buf, sizeof buf, stdin)) exit(1);
    for (char *c = buf; *c; ++c) {
        if (isspace((unsigned char)*c)) continue;
        return tolower((unsigned char)*c) == 'y';
    }
    return default_yes;
}

int main(void)
{
    double bod = 0, cod = 0;
    int have_bod = 0, have_cod = 0;

    printf("=== Water Test Calculator: BOD & COD ===\n");

    if (ask_yn("\nCalculate BOD?", 1)) {
        double d1 = ask("Initial DO, D1 (mg/L)", 0, 0);
        double d2 = ask("DO after 5 days, D2 (mg/L)", 0, 0);
        double sample = ask("Sample volume (mL)", 0, 0);
        double bottle = ask("BOD bottle volume (mL)", 1, 300.0);
        int seed = 0;
        double b1 = 0, b2 = 0, f = 1.0;
        if (ask_yn("Use seed correction?", 0)) {
            seed = 1;
            b1 = ask("Seed control initial DO, B1 (mg/L)", 0, 0);
            b2 = ask("Seed control final DO, B2 (mg/L)", 0, 0);
            f  = ask("Seed factor f", 1, 1.0);
        }
        if (calculate_bod(d1, d2, sample, bottle, seed, b1, b2, f, &bod) == 0) {
            have_bod = 1;
            printf("\nBOD5 = %.2f mg/L\n", bod);
        }
    }

    if (ask_yn("\nCalculate COD?", 1)) {
        double blank = ask("Blank titre, A (mL FAS)", 0, 0);
        double titre = ask("Sample titre, B (mL FAS)", 0, 0);
        double m     = ask("FAS molarity, M", 0, 0);
        double vol   = ask("Sample volume (mL)", 1, 20.0);
        if (calculate_cod(blank, titre, m, vol, &cod) == 0) {
            have_cod = 1;
            printf("\nCOD = %.2f mg O2/L\n", cod);
        }
    }

    if (have_bod && have_cod && cod != 0.0) {
        double ratio;
        const char *note = classify(bod, cod, &ratio);
        printf("\nBOD/COD ratio = %.2f -> %s\n", ratio, note);
    }
    return 0;
}
