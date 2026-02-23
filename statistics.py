from __future__ import annotations
from typing import Iterable, List, Dict


class StatisticsCalculator:
    """Encapsulates basic statistics for a list of integers."""

    def __init__(self, data: Iterable[int]) -> None:
        data_list = list(data)
        if not data_list:
            raise ValueError("data must not be empty")
        # Ensure all items are integers (defensive)
        for x in data_list:
            if not isinstance(x, int):
                raise TypeError(f"All items must be int, got {type(x).__name__}")
        self._data: List[int] = data_list

    # --- Core operations ---

    def mean(self) -> float:
        """Return the arithmetic mean as a float. O(n)."""
        return sum(self._data) / len(self._data)

    def median(self) -> float:
        """Return the median as a float. O(n log n) due to sorting."""
        s = sorted(self._data)
        n = len(s)
        mid = n // 2
        if n % 2 == 1:
            return float(s[mid])
        # even length → average of the two middle values
        return (s[mid - 1] + s[mid]) / 2.0

    def mode(self) -> List[int]:
        """
        Return all mode values (sorted) as a list. O(n).
        If all values occur exactly once, every value is a mode.
        """
        counts: Dict[int, int] = {}
        for x in self._data:
            counts[x] = counts.get(x, 0) + 1
        maxf = max(counts.values())
        return sorted([k for k, v in counts.items() if v == maxf])

    # --- Convenience ---

    def describe(self) -> str:
        """Return a human-readable summary."""
        m = self.mean()
        med = self.median()
        modes = self.mode()
        return (
            f"Count : {len(self._data)}\n"
            f"Mean  : {m:.6f}\n"
            f"Median: {med:.6f}\n"
            f"Mode  : {modes}\n"
        )


# --- CLI entry point ---------------------------------------------------------

def _parse_cli(argv: List[str]) -> List[int]:
    if len(argv) <= 1:
        raise SystemExit(
            f"Usage: {argv[0]} <int1> <int2> ...\n"
            f"Example: {argv[0]} 1 2 2 3 4 4 4 5"
        )
    try:
        return [int(x) for x in argv[1:]]
    except ValueError as e:
        raise SystemExit(f"Invalid integer in input: {e}") from e


if __name__ == "__main__":
    import sys

    numbers = _parse_cli(sys.argv)
    calc = StatisticsCalculator(numbers)
    print(calc.describe(), end="")
