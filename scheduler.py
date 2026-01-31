
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple
import random
import sys

DAYS = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
SHIFTS = ["morning", "afternoon", "evening"]
SHIFT_SET = set(SHIFTS)

@dataclass
class Config:
    min_per_shift: int = 2
    max_per_shift: Optional[int] = 4 
    max_days_per_employee: int = 5
    random_seed: int = 42

Schedule = Dict[str, Dict[str, List[str]]]
Preferences = Dict[str, Dict[str, List[str]]]

def normalize_preferences(raw_prefs: Dict[str, Dict[str, object]]) -> Preferences:
    """
    Normalize raw preferences so that each employee/day maps to a ranked list of shifts.
    Accepts strings or lists per day; validates shift names.
    """
    prefs: Preferences = {}
    for emp, per_day in raw_prefs.items():
        prefs[emp] = {}
        for day in DAYS:
            raw = per_day.get(day, [])
            if isinstance(raw, str):
                ranked = [raw]
            elif isinstance(raw, list):
                ranked = [s for s in raw if isinstance(s, str)]
            else:
                ranked = []
            ranked = [s.lower().strip() for s in ranked if s.lower().strip() in SHIFT_SET]
            prefs[emp][day] = ranked
    return prefs

def empty_schedule() -> Schedule:
    return {day: {shift: [] for shift in SHIFTS} for day in DAYS}

def feasible_or_raise(employees: List[str], cfg: Config):
    required = len(DAYS) * len(SHIFTS) * cfg.min_per_shift
    supply = len(employees) * cfg.max_days_per_employee
    if supply < required:
        raise ValueError(
            f"Infeasible: need at least {required} total shift assignments, "
            f"but employee supply caps at {supply}. Consider adding ~{-(supply - required) // cfg.max_days_per_employee + 1} more employees "
            f"or increasing max_days_per_employee."
        )

def schedule_employees(
    employees: List[str],
    raw_preferences: Dict[str, Dict[str, object]],
    cfg: Config = Config(),
) -> Tuple[Schedule, List[str]]:
    """
    Build a weekly schedule honoring preferences as much as possible, then ensure
    min staffing with random backfill. Returns (schedule, warnings).
    """
    random.seed(cfg.random_seed)
    employees = list(dict.fromkeys([e.strip() for e in employees if e.strip()]))  # de-dup & clean
    if not employees:
        raise ValueError("No employees provided.")

    feasible_or_raise(employees, cfg)

    prefs = normalize_preferences(raw_preferences)
    sched: Schedule = empty_schedule()
    assigned_on_day: Dict[str, set] = {d: set() for d in DAYS}
    days_worked: Dict[str, int] = {e: 0 for e in employees}
    warnings: List[str] = []

    def shift_has_capacity(day: str, shift: str) -> bool:
        if not cfg.max_per_shift or cfg.max_per_shift <= 0:
            return True
        return len(sched[day][shift]) < cfg.max_per_shift

  
    carry_over_next_day: Dict[str, List[str]] = {d: [] for d in DAYS}

    for di, day in enumerate(DAYS):
        order = carry_over_next_day[day] + [e for e in employees if e not in carry_over_next_day[day]]

        for rank in range(3):
            for emp in order:
                if emp in assigned_on_day[day]:
                    continue
                if days_worked[emp] >= cfg.max_days_per_employee:
                    continue

                ranked = prefs.get(emp, {}).get(day, [])
                if rank >= len(ranked):
                    continue
                target = ranked[rank]
                if target not in SHIFT_SET:
                    continue

                # Enforce 1 shift/day and capacity
                if shift_has_capacity(day, target):
                    sched[day][target].append(emp)
                    assigned_on_day[day].add(emp)
                    days_worked[emp] += 1

        for emp in order:
            if emp in assigned_on_day[day]:
                continue
            if days_worked[emp] >= cfg.max_days_per_employee:
                continue

            if prefs.get(emp, {}).get(day):
                placed = False
                seen = set(prefs[emp][day])
                for s in prefs[emp][day] + [s for s in SHIFTS if s not in seen]:
                    if shift_has_capacity(day, s):
                        sched[day][s].append(emp)
                        assigned_on_day[day].add(emp)
                        days_worked[emp] += 1
                        placed = True
                        break
                if not placed:
                    if di < len(DAYS) - 1:
                        carry_over_next_day[DAYS[di + 1]].append(emp)

    for day in DAYS:
        for shift in SHIFTS:
            need = cfg.min_per_shift - len(sched[day][shift])
            if need <= 0:
                continue
                
            candidates = [e for e in employees
                          if e not in assigned_on_day[day] and days_worked[e] < cfg.max_days_per_employee]
            random.shuffle(candidates)

            added = 0
            for emp in candidates:
                if cfg.max_per_shift and cfg.max_per_shift > 0 and len(sched[day][shift]) >= cfg.max_per_shift:
                    break  
                sched[day][shift].append(emp)
                assigned_on_day[day].add(emp)
                days_worked[emp] += 1
                added += 1
                if added >= need:
                    break

            if len(sched[day][shift]) < cfg.min_per_shift:
                warnings.append(
                    f"Warning: Could not meet min staffing for {day} {shift} "
                    f"({len(sched[day][shift])}/{cfg.min_per_shift}). Consider more staff or relaxing caps."
                )

    if cfg.max_per_shift and cfg.max_per_shift > 0:
        for day in DAYS:
            for shift in SHIFTS:
                while len(sched[day][shift]) > cfg.max_per_shift:
                    emp = sched[day][shift].pop()  
                    assigned_on_day[day].remove(emp)
                    days_worked[emp] -= 1
                    placed = False
                    for s in [s for s in SHIFTS if s != shift]:
                        if len(sched[day][s]) < cfg.max_per_shift and emp not in assigned_on_day[day]:
                            sched[day][s].append(emp)
                            assigned_on_day[day].add(emp)
                            days_worked[emp] += 1
                            placed = True
                            break
                    if not placed:
                        di = DAYS.index(day)
                        if di < len(DAYS) - 1:
                            next_day = DAYS[di + 1]
                            for s in SHIFTS:
                                if emp in assigned_on_day[next_day]:
                                    continue
                                if not cfg.max_per_shift or len(sched[next_day][s]) < cfg.max_per_shift:
                                    sched[next_day][s].append(emp)
                                    assigned_on_day[next_day].add(emp)
                                    days_worked[emp] += 1
                                    placed = True
                                    break
                    if not placed:
                        warnings.append(f"Note: Could not relocate {emp} from {day} {shift}; leaving unassigned.")

    return sched, warnings

def print_schedule(schedule: Schedule):
    print("\n=== Final Weekly Schedule ===")
    for day in DAYS:
        print(f"\n{day}:")
        for shift in SHIFTS:
            names = ", ".join(sorted(schedule[day][shift]))
            print(f"  - {shift.title():<9} : {names if names else '(none)'}")

def example_dataset():
    employees = [
        "Alice", "Bob", "Charlie", "Diana", "Evan",
        "Farah", "Grace", "Henry", "Iris", "Jamal"
    ]  
    prefs = {
        "Alice":   {"Mon": ["morning","afternoon"], "Tue": "morning", "Wed": "morning", "Thu": "morning", "Fri": "morning"},
        "Bob":     {"Mon": "evening", "Tue": ["evening","afternoon"], "Wed": "evening", "Thu": "evening", "Fri": "evening"},
        "Charlie": {"Mon": "afternoon", "Tue": "afternoon", "Wed": "afternoon", "Thu": "afternoon", "Fri": "afternoon"},
        "Diana":   {"Mon": "morning", "Tue": "morning", "Wed": "evening", "Sat": ["morning","evening"]},
        "Evan":    {"Tue": "morning", "Wed": "morning", "Thu": "evening", "Sun": "morning"},
        "Farah":   {"Mon": "evening", "Wed": ["morning","evening"], "Fri": "afternoon", "Sun": "evening"},
        "Grace":   {"Thu": ["morning","afternoon"], "Fri": "morning", "Sat": "afternoon"},
        "Henry":   {"Mon": "afternoon", "Tue": ["morning","afternoon"], "Sat": "evening"},
        "Iris":    {"Wed": "evening", "Thu": "evening", "Fri": "evening"},
        "Jamal":   {"Tue": "afternoon", "Thu": "morning", "Sun": ["afternoon","morning"]},
    }
    return employees, prefs

if __name__ == "__main__":
    employees, prefs = example_dataset()
    cfg = Config(min_per_shift=2, max_per_shift=4, max_days_per_employee=5, random_seed=7)

    try:
        schedule, warnings = schedule_employees(employees, prefs, cfg)
        print_schedule(schedule)
        if warnings:
            print("\nNotes & Warnings:")
            for w in warnings:
                print(" -", w)
    except ValueError as e:
        print("Error:", e)
        sys.exit(1)
