import threading
import json
from datetime import datetime
from typing import List, Dict

# -----------------------------
# Data Storage (Python Dictionary)
# -----------------------------
expenses: List[Dict] = []

DATA_FILE = "expenses.json"


# -----------------------------
# Utility Functions
# -----------------------------
def parse_date(date_str: str) -> datetime:
    """Convert string to datetime object"""
    return datetime.strptime(date_str, "%Y-%m-%d")


def save_expenses():
    """Save expenses to file (runs in background thread)"""
    with open(DATA_FILE, "w") as file:
        json.dump(expenses, file, default=str, indent=4)
    print("✔ Expenses saved successfully.\n")


def load_expenses():
    """Load expenses from file"""
    global expenses
    try:
        with open(DATA_FILE, "r") as file:
            expenses = json.load(file)
            # Convert date strings back to datetime objects
            for exp in expenses:
                exp["date"] = parse_date(exp["date"])
    except FileNotFoundError:
        expenses = []


# -----------------------------
# Core Features
# -----------------------------
def add_expense():
    """Add a new expense"""
    try:
        date = parse_date(input("Enter date (YYYY-MM-DD): "))
        amount = float(input("Enter amount: "))
        category = input("Enter category: ").strip()
        description = input("Enter description: ").strip()

        expense = {
            "date": date,
            "amount": amount,
            "category": category,
            "description": description
        }

        expenses.append(expense)

        # Save data using concurrency (non-blocking)
        threading.Thread(target=save_expenses).start()

        print("✔ Expense added successfully.\n")

    except ValueError:
        print("Invalid input. Please try again.\n")


def view_expenses():
    """View all expenses"""
    if not expenses:
        print("No expenses recorded.\n")
        return

    for exp in expenses:
        print(
            f"{exp['date'].date()} | "
            f"${exp['amount']:.2f} | "
            f"{exp['category']} | "
            f"{exp['description']}"
        )
    print()


def filter_by_category():
    """Filter expenses by category"""
    category = input("Enter category to filter: ").strip()

    filtered = [exp for exp in expenses if exp["category"].lower() == category.lower()]

    if not filtered:
        print("No expenses found for this category.\n")
        return

    for exp in filtered:
        print(f"{exp['date'].date()} | ${exp['amount']:.2f} | {exp['description']}")
    print()


def filter_by_date_range():
    """Filter expenses by date range"""
    try:
        start = parse_date(input("Start date (YYYY-MM-DD): "))
        end = parse_date(input("End date (YYYY-MM-DD): "))

        filtered = [exp for exp in expenses if start <= exp["date"] <= end]

        if not filtered:
            print("No expenses found in this date range.\n")
            return

        for exp in filtered:
            print(
                f"{exp['date'].date()} | "
                f"${exp['amount']:.2f} | "
                f"{exp['category']} | "
                f"{exp['description']}"
            )
        print()

    except ValueError:
        print("❌ Invalid date format.\n")


def show_summary():
    """Show total expenses by category and overall"""
    category_totals: Dict[str, float] = {}
    total = 0.0

    for exp in expenses:
        category = exp["category"]
        amount = exp["amount"]

        category_totals[category] = category_totals.get(category, 0) + amount
        total += amount

    print("\nExpense Summary:")
    for category, amount in category_totals.items():
        print(f"{category}: ${amount:.2f}")

    print(f"Overall Total: ${total:.2f}\n")


# -----------------------------
# Main Menu
# -----------------------------
def main():
    load_expenses()

    while True:
        print("=== Expense Tracker ===")
        print("1. Add Expense")
        print("2. View Expenses")
        print("3. Filter by Category")
        print("4. Filter by Date Range")
        print("5. Show Summary")
        print("6. Exit")

        choice = input("Choose an option: ").strip()

        if choice == "1":
            add_expense()
        elif choice == "2":
            view_expenses()
        elif choice == "3":
            filter_by_category()
        elif choice == "4":
            filter_by_date_range()
        elif choice == "5":
            show_summary()
        elif choice == "6":
            print("Goodbye!")
            break
        else:
            print("Invalid choice. Try again.\n")


if __name__ == "__main__":
    main()
