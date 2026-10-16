// BOD and COD calculator for water quality testing (C++17 version)
//
// BOD (5-day, dilution method, APHA 5210 B)
//   Without seed: BOD = (D1 - D2) / P
//   With seed:    BOD = ((D1 - D2) - (B1 - B2) * f) / P
//   P = sample volume / bottle volume
//
// COD (dichromate reflux, APHA 5220)
//   COD (mg O2/L) = (A - B) * M * 8000 / V
//
// Build: g++ -std=c++17 -Wall -o bod_cod bod_cod.cpp

#include <cctype>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

struct Seed {
    double b1;
    double b2;
    double factor = 1.0;
};

double calculateBod(double d1, double d2, double sampleMl, double bottleMl = 300.0,
                    const std::optional<Seed>& seed = std::nullopt)
{
    if (sampleMl <= 0 || bottleMl <= 0 || sampleMl > bottleMl)
        throw std::invalid_argument("Sample volume must be > 0 and <= bottle volume.");

    const double p = sampleMl / bottleMl;
    const double depletion = d1 - d2;

    if (depletion < 2.0)
        std::cout << "Warning: DO depletion < 2 mg/L. Use a larger sample volume.\n";
    if (d2 < 1.0)
        std::cout << "Warning: residual DO < 1 mg/L. Use a smaller sample volume.\n";

    const double seedCorrection = seed ? (seed->b1 - seed->b2) * seed->factor : 0.0;
    return (depletion - seedCorrection) / p;
}

double calculateCod(double blankMl, double sampleTitreMl, double fasMolarity, double sampleMl)
{
    if (sampleMl <= 0)
        throw std::invalid_argument("Sample volume must be > 0.");
    if (blankMl < sampleTitreMl)
        std::cout << "Warning: blank titre is smaller than sample titre. Check readings.\n";
    return (blankMl - sampleTitreMl) * fasMolarity * 8000.0 / sampleMl;
}

// Standardise FAS: M = vol K2Cr2O7 * normality / vol FAS (0.04167 M x 6 = 0.25 N)
double fasMolarity(double k2cr2o7Ml, double fasMl, double k2cr2o7Molarity = 0.04167)
{
    return k2cr2o7Ml * (k2cr2o7Molarity * 6.0) / fasMl;
}

std::pair<double, std::string> classify(double bod, double cod)
{
    const double ratio = (cod != 0.0) ? bod / cod : 0.0;
    if (ratio > 0.5)
        return {ratio, "Easily biodegradable (BOD/COD > 0.5)"};
    if (ratio >= 0.3)
        return {ratio, "Moderately biodegradable (BOD/COD 0.3-0.5)"};
    return {ratio, "Poorly biodegradable / possible toxic or inorganic load (< 0.3)"};
}

// Prompt for a double; empty input returns the default if one is given.
double ask(const std::string& prompt, std::optional<double> def = std::nullopt)
{
    std::string line;
    for (;;) {
        std::cout << prompt;
        if (def) std::cout << " [" << *def << "]";
        std::cout << ": ";
        if (!std::getline(std::cin, line)) std::exit(1);

        if (line.find_first_not_of(" \t\r\n") == std::string::npos && def)
            return *def;
        try {
            std::size_t pos = 0;
            double v = std::stod(line, &pos);
            if (line.find_first_not_of(" \t\r\n", pos) == std::string::npos)
                return v;
        } catch (const std::exception&) {}
        std::cout << "Invalid input, try again.\n";
    }
}

bool askYesNo(const std::string& prompt, bool defaultYes)
{
    std::cout << prompt << " (y/n) [" << (defaultYes ? 'y' : 'n') << "]: ";
    std::string line;
    if (!std::getline(std::cin, line)) std::exit(1);
    for (unsigned char c : line) {
        if (std::isspace(c)) continue;
        return std::tolower(c) == 'y';
    }
    return defaultYes;
}

int main()
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Water Test Calculator: BOD & COD ===\n";

    std::optional<double> bod, cod;

    try {
        if (askYesNo("\nCalculate BOD?", true)) {
            const double d1 = ask("Initial DO, D1 (mg/L)");
            const double d2 = ask("DO after 5 days, D2 (mg/L)");
            const double sample = ask("Sample volume (mL)");
            const double bottle = ask("BOD bottle volume (mL)", 300.0);

            std::optional<Seed> seed;
            if (askYesNo("Use seed correction?", false)) {
                Seed s;
                s.b1 = ask("Seed control initial DO, B1 (mg/L)");
                s.b2 = ask("Seed control final DO, B2 (mg/L)");
                s.factor = ask("Seed factor f", 1.0);
                seed = s;
            }
            bod = calculateBod(d1, d2, sample, bottle, seed);
            std::cout << "\nBOD5 = " << *bod << " mg/L\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    try {
        if (askYesNo("\nCalculate COD?", true)) {
            const double blank = ask("Blank titre, A (mL FAS)");
            const double titre = ask("Sample titre, B (mL FAS)");
            const double m = ask("FAS molarity, M");
            const double vol = ask("Sample volume (mL)", 20.0);
            cod = calculateCod(blank, titre, m, vol);
            std::cout << "\nCOD = " << *cod << " mg O2/L\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    if (bod && cod && *cod != 0.0) {
        auto [ratio, note] = classify(*bod, *cod);
        std::cout << "\nBOD/COD ratio = " << ratio << " -> " << note << "\n";
    }
    return 0;
}
