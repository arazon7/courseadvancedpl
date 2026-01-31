#include <algorithm>
#include <cctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

static const vector<string> DAYS = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
static const vector<string> SHIFTS = {"morning","afternoon","evening"};
static const unordered_set<string> SHIFT_SET = {"morning","afternoon","evening"};


struct Config {
    int min_per_shift = 2;
    int max_per_shift = 4;            
    int max_days_per_employee = 5;
    unsigned int random_seed = 42;
};

using Schedule = unordered_map<string, unordered_map<string, vector<string>>>;
using Preferences = unordered_map<string, unordered_map<string, vector<string>>>;


using PrefValue = variant<string, vector<string>>;
using RawPreferences = unordered_map<string, unordered_map<string, PrefValue>>;



static inline string ltrim(const string& s) {
    size_t i = 0;
    while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) ++i;
    return s.substr(i);
}

static inline string rtrim(const string& s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && isspace(static_cast<unsigned char>(s[i-1]))) --i;
    return s.substr(0, i);
}

static inline string trim(const string& s) {
    return rtrim(ltrim(s));
}

static inline string to_lower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c){ return static_cast<char>(tolower(c)); });
    return s;
}

static vector<string> unique_cleaned(const vector<string>& names) {
    vector<string> out;
    unordered_set<string> seen;
    out.reserve(names.size());
    for (auto n : names) {
        n = trim(n);
        if (n.empty()) continue;
        if (seen.insert(n).second) out.push_back(n);
    }
    return out;
}



Preferences normalize_preferences(const RawPreferences& raw_prefs) {
    Preferences prefs;

    for (const auto& [emp, per_day] : raw_prefs) {
        auto& emp_map = prefs[emp];
        for (const auto& day : DAYS) {
            vector<string> ranked;

            auto it = per_day.find(day);
            if (it != per_day.end()) {
                const auto& val = it->second;
                if (holds_alternative<string>(val)) {
                    string s = to_lower(trim(get<string>(val)));
                    if (!s.empty() && SHIFT_SET.count(s)) {
                        ranked.push_back(s);
                    }
                } else if (holds_alternative<vector<string>>(val)) {
                    const auto& vec = get<vector<string>>(val);
                    for (auto s : vec) {
                        s = to_lower(trim(s));
                        if (!s.empty() && SHIFT_SET.count(s)) {
                            ranked.push_back(s);
                        }
                    }
                }
            }
            
            emp_map[day] = ranked;
        }
    }

    return prefs;
}

Schedule empty_schedule() {
    Schedule sched;
    for (const auto& day : DAYS) {
        auto& per_shift = sched[day];
        for (const auto& s : SHIFTS) per_shift[s] = {};
    }
    return sched;
}

void feasible_or_raise(const vector<string>& employees, const Config& cfg) {
    int required = static_cast<int>(DAYS.size() * SHIFTS.size() * cfg.min_per_shift);
    int supply = static_cast<int>(employees.size() * cfg.max_days_per_employee);
    if (supply < required) {
        int deficit = required - supply;
        int need_more = deficit / cfg.max_days_per_employee + ((deficit % cfg.max_days_per_employee) ? 1 : 0);
        throw runtime_error(
            "Infeasible: need at least " + to_string(required) + " total shift assignments, "
            "but employee supply caps at " + to_string(supply) + ". Consider adding ~" + to_string(need_more) +
            " more employees or increasing max_days_per_employee."
        );
    }
}

pair<Schedule, vector<string>> schedule_employees(
    vector<string> employees,
    const RawPreferences& raw_preferences,
    Config cfg = Config()
) {
    
    mt19937 rng(cfg.random_seed);

    
    employees = unique_cleaned(employees);
    if (employees.empty()) {
        throw invalid_argument("No employees provided.");
    }

    feasible_or_raise(employees, cfg);

    Preferences prefs = normalize_preferences(raw_preferences);
    Schedule sched = empty_schedule();

    unordered_map<string, unordered_set<string>> assigned_on_day;
    for (const auto& d : DAYS) assigned_on_day[d] = {};

    unordered_map<string, int> days_worked;
    for (const auto& e : employees) days_worked[e] = 0;

    vector<string> warnings;

    auto shift_has_capacity = [&](const string& day, const string& shift) -> bool {
        if (cfg.max_per_shift <= 0) return true;
        return static_cast<int>(sched[day][shift].size()) < cfg.max_per_shift;
    };

    
    unordered_map<string, vector<string>> carry_over_next_day;
    for (const auto& d : DAYS) carry_over_next_day[d] = {};

    for (size_t di = 0; di < DAYS.size(); ++di) {
        const string& day = DAYS[di];

       
        vector<string> order;
        unordered_set<string> carry_set(carry_over_next_day[day].begin(), carry_over_next_day[day].end());
        order.insert(order.end(), carry_over_next_day[day].begin(), carry_over_next_day[day].end());
        for (const auto& e : employees) {
            if (!carry_set.count(e)) order.push_back(e);
        }

       
        for (int rank = 0; rank < 3; ++rank) {
            for (const auto& emp : order) {
                if (assigned_on_day[day].count(emp)) continue;
                if (days_worked[emp] >= cfg.max_days_per_employee) continue;

                const auto& ranked = prefs[emp][day]; 
                if (rank >= static_cast<int>(ranked.size())) continue;
                const string& target = ranked[rank];
                if (!SHIFT_SET.count(target)) continue;

                if (shift_has_capacity(day, target)) {
                    sched[day][target].push_back(emp);
                    assigned_on_day[day].insert(emp);
                    days_worked[emp] += 1;
                }
            }
        }

     
        for (const auto& emp : order) {
            if (assigned_on_day[day].count(emp)) continue;
            if (days_worked[emp] >= cfg.max_days_per_employee) continue;

            const auto& ranked = prefs[emp][day];
            if (!ranked.empty()) {
                bool placed = false;
                unordered_set<string> seen(ranked.begin(), ranked.end());

                vector<string> try_order = ranked; 
                for (const auto& s : SHIFTS) {
                    if (!seen.count(s)) try_order.push_back(s);
                }

                for (const auto& s : try_order) {
                    if (shift_has_capacity(day, s)) {
                        sched[day][s].push_back(emp);
                        assigned_on_day[day].insert(emp);
                        days_worked[emp] += 1;
                        placed = true;
                        break;
                    }
                }
                if (!placed) {
                    if (di + 1 < DAYS.size()) {
                        carry_over_next_day[DAYS[di + 1]].push_back(emp);
                    }
                }
            }
        }
    }


    for (const auto& day : DAYS) {
        for (const auto& shift : SHIFTS) {
            int have = static_cast<int>(sched[day][shift].size());
            int need = cfg.min_per_shift - have;
            if (need <= 0) continue;

            vector<string> candidates;
            candidates.reserve(employees.size());
            for (const auto& e : employees) {
                if (!assigned_on_day[day].count(e) && days_worked[e] < cfg.max_days_per_employee) {
                    candidates.push_back(e);
                }
            }
            shuffle(candidates.begin(), candidates.end(), rng);

            int added = 0;
            for (const auto& emp : candidates) {
                if (cfg.max_per_shift > 0 && static_cast<int>(sched[day][shift].size()) >= cfg.max_per_shift) {
                    break; 
                }
                sched[day][shift].push_back(emp);
                assigned_on_day[day].insert(emp);
                days_worked[emp] += 1;
                added += 1;
                if (added >= need) break;
            }

            if (static_cast<int>(sched[day][shift].size()) < cfg.min_per_shift) {
                warnings.push_back(
                    "Warning: Could not meet min staffing for " + day + " " + shift +
                    " (" + to_string(sched[day][shift].size()) + "/" + to_string(cfg.min_per_shift) +
                    "). Consider more staff or relaxing caps."
                );
            }
        }
    }

    if (cfg.max_per_shift > 0) {
        for (const auto& day : DAYS) {
            for (const auto& shift : SHIFTS) {
                auto& vec = sched[day][shift];
                while (static_cast<int>(vec.size()) > cfg.max_per_shift) {
                    string emp = vec.back();
                    vec.pop_back();
                    if (assigned_on_day[day].count(emp)) {
                        assigned_on_day[day].erase(emp);
                        days_worked[emp] -= 1;
                    }
                    bool placed = false;

                    for (const auto& s2 : SHIFTS) {
                        if (s2 == shift) continue;
                        if (static_cast<int>(sched[day][s2].size()) < cfg.max_per_shift &&
                            !assigned_on_day[day].count(emp)) {
                            sched[day][s2].push_back(emp);
                            assigned_on_day[day].insert(emp);
                            days_worked[emp] += 1;
                            placed = true;
                            break;
                        }
                    }

                    if (!placed) {
                        auto itd = find(DAYS.begin(), DAYS.end(), day);
                        if (itd != DAYS.end()) {
                            size_t di = static_cast<size_t>(distance(DAYS.begin(), itd));
                            if (di + 1 < DAYS.size()) {
                                const string& next_day = DAYS[di + 1];
                                for (const auto& s : SHIFTS) {
                                    if (assigned_on_day[next_day].count(emp)) continue;
                                    if (cfg.max_per_shift <= 0 ||
                                        static_cast<int>(sched[next_day][s].size()) < cfg.max_per_shift) {
                                        sched[next_day][s].push_back(emp);
                                        assigned_on_day[next_day].insert(emp);
                                        days_worked[emp] += 1;
                                        placed = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (!placed) {
                        warnings.push_back(
                            "Note: Could not relocate " + emp + " from " + day + " " + shift +
                            "; leaving unassigned."
                        );
                    }
                }
            }
        }
    }

    return {sched, warnings};
}

void print_schedule(const Schedule& schedule) {
    cout << "\n=== Final Weekly Schedule ===\n";
    for (const auto& day : DAYS) {
        cout << "\n" << day << ":\n";
        for (const auto& shift : SHIFTS) {
            vector<string> names = schedule.at(day).at(shift);
            sort(names.begin(), names.end());
            cout << "  - " << setw(9) << left << string(1, toupper(shift[0])) + shift.substr(1) << " : ";
            if (names.empty()) {
                cout << "(none)";
            } else {
                for (size_t i = 0; i < names.size(); ++i) {
                    if (i) cout << ", ";
                    cout << names[i];
                }
            }
            cout << "\n";
        }
    }
}


pair<vector<string>, RawPreferences> example_dataset() {
    vector<string> employees = {
        "Alice", "Bob", "Charlie", "Diana", "Evan",
        "Farah", "Grace", "Henry", "Iris", "Jamal"
    };

    RawPreferences prefs;

    prefs["Alice"]["Mon"] = vector<string>{"morning","afternoon"};
    prefs["Alice"]["Tue"] = string("morning");
    prefs["Alice"]["Wed"] = string("morning");
    prefs["Alice"]["Thu"] = string("morning");
    prefs["Alice"]["Fri"] = string("morning");

    prefs["Bob"]["Mon"] = string("evening");
    prefs["Bob"]["Tue"] = vector<string>{"evening","afternoon"};
    prefs["Bob"]["Wed"] = string("evening");
    prefs["Bob"]["Thu"] = string("evening");
    prefs["Bob"]["Fri"] = string("evening");

    prefs["Charlie"]["Mon"] = string("afternoon");
    prefs["Charlie"]["Tue"] = string("afternoon");
    prefs["Charlie"]["Wed"] = string("afternoon");
    prefs["Charlie"]["Thu"] = string("afternoon");
    prefs["Charlie"]["Fri"] = string("afternoon");

    prefs["Diana"]["Mon"] = string("morning");
    prefs["Diana"]["Tue"] = string("morning");
    prefs["Diana"]["Wed"] = string("evening");
    prefs["Diana"]["Sat"] = vector<string>{"morning","evening"};

    prefs["Evan"]["Tue"] = string("morning");
    prefs["Evan"]["Wed"] = string("morning");
    prefs["Evan"]["Thu"] = string("evening");
    prefs["Evan"]["Sun"] = string("morning");

    prefs["Farah"]["Mon"] = string("evening");
    prefs["Farah"]["Wed"] = vector<string>{"morning","evening"};
    prefs["Farah"]["Fri"] = string("afternoon");
    prefs["Farah"]["Sun"] = string("evening");

    prefs["Grace"]["Thu"] = vector<string>{"morning","afternoon"};
    prefs["Grace"]["Fri"] = string("morning");
    prefs["Grace"]["Sat"] = string("afternoon");

    prefs["Henry"]["Mon"] = string("afternoon");
    prefs["Henry"]["Tue"] = vector<string>{"morning","afternoon"};
    prefs["Henry"]["Sat"] = string("evening");

    prefs["Iris"]["Wed"] = string("evening");
    prefs["Iris"]["Thu"] = string("evening");
    prefs["Iris"]["Fri"] = string("evening");

    prefs["Jamal"]["Tue"] = string("afternoon");
    prefs["Jamal"]["Thu"] = string("morning");
    prefs["Jamal"]["Sun"] = vector<string>{"afternoon","morning"};

    return {employees, prefs};
}


int main() {
    try {
        auto [employees, prefs] = example_dataset();
        Config cfg;
        cfg.min_per_shift = 2;
        cfg.max_per_shift = 4;  
        cfg.max_days_per_employee = 5;
        cfg.random_seed = 7;

        auto [schedule, warnings] = schedule_employees(employees, prefs, cfg);
        print_schedule(schedule);

        if (!warnings.empty()) {
            cout << "\nNotes & Warnings:\n";
            for (const auto& w : warnings) {
                cout << " - " << w << "\n";
            }
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
