#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <numeric>

using std::cout;
using std::endl;
using std::fixed;
using std::make_shared;
using std::setprecision;
using std::shared_ptr;
using std::string;
using std::vector;

// ---------------------------- Base Class ----------------------------
class Ride {
protected:
    string rideID;
    string pickupLocation;
    string dropoffLocation;
    double distance; // miles

public:
    Ride(string id, string pickup, string dropoff, double miles)
        : rideID(std::move(id)),
          pickupLocation(std::move(pickup)),
          dropoffLocation(std::move(dropoff)),
          distance(miles) {}

    virtual ~Ride() = default;

    // Polymorphic interface
    virtual double fare() const = 0;
    virtual string rideType() const = 0;

    // Non-virtual helper that uses dynamic dispatch internally
    virtual void rideDetails(std::ostream& os) const {
        os << "[" << rideType() << "] " << rideID
           << " | " << pickupLocation << " → " << dropoffLocation
           << " | " << distance << " mi"
           << " | fare: $" << fixed << setprecision(2) << fare();
    }

    // Read-only accessors (encapsulation: no direct mutation)
    const string& id() const { return rideID; }
    const string& pickup() const { return pickupLocation; }
    const string& dropoff() const { return dropoffLocation; }
    double miles() const { return distance; }
};

// ---------------------- Derived Classes (Inheritance) ----------------------
class StandardRide : public Ride {
    // Example pricing: $3.00 base + $1.50 per mile
public:
    using Ride::Ride; // inherit constructor
    double fare() const override {
        return 3.00 + miles() * 1.50;
    }
    string rideType() const override { return "Standard"; }
};

class PremiumRide : public Ride {
    // Example pricing: $5.00 base + $2.50 per mile × surge multiplier
    double surgeMultiplier;

public:
    PremiumRide(string id, string pickup, string dropoff, double miles, double surge = 1.0)
        : Ride(std::move(id), std::move(pickup), std::move(dropoff), miles),
          surgeMultiplier(surge) {}

    double fare() const override {
        return 5.00 + miles() * 2.50 * surgeMultiplier;
    }
    string rideType() const override { return "Premium"; }
};

// ----------------------------- Driver (Encapsulation) -----------------------------
class Driver {
    string driverID;
    string name;
    double rating; // 1..5
    // Encapsulation: keep the list private; expose behavior, not representation
    vector<shared_ptr<Ride>> assignedRides;

public:
    Driver(string id, string n, double r)
        : driverID(std::move(id)), name(std::move(n)), rating(r) {}

    void addRide(const shared_ptr<Ride>& ride) {
        assignedRides.push_back(ride);
    }

    double totalEarnings() const {
        double sum = 0.0;
        for (const auto& r : assignedRides) sum += r->fare();
        return sum;
    }

    void getDriverInfo(std::ostream& os) const {
        os << "Driver " << name << " (" << driverID << ")"
           << " | rating: " << fixed << setprecision(2) << rating << "\n";
        os << "Assigned rides:\n";
        for (const auto& r : assignedRides) {
            os << "  - ";
            r->rideDetails(os);
            os << "\n";
        }
        os << "Total earnings: $" << fixed << setprecision(2) << totalEarnings() << "\n";
    }
};

// ------------------------------ Rider (Encapsulation) ------------------------------
class Rider {
    string riderID;
    string name;
    vector<shared_ptr<Ride>> requestedRides;

public:
    Rider(string id, string n) : riderID(std::move(id)), name(std::move(n)) {}

    void requestRide(const shared_ptr<Ride>& ride) {
        requestedRides.push_back(ride);
    }

    void viewRides(std::ostream& os) const {
        os << "Rider " << name << " (" << riderID << ") ride history:\n";
        for (const auto& r : requestedRides) {
            os << "  - ";
            r->rideDetails(os);
            os << "\n";
        }
    }
};

// --------------------------------- Demo (Polymorphism) ---------------------------------
int main() {
    // Create rides of different types
    auto r1 = make_shared<StandardRide>("R1001", "Downtown", "Airport", 15.2);
    auto r2 = make_shared<PremiumRide>("R1002", "UT Austin", "The Domain", 8.5, 1.30);
    auto r3 = make_shared<StandardRide>("R1003", "Capitol", "Zilker Park", 5.1);
    auto r4 = make_shared<PremiumRide>("R1004", "South Congress", "Downtown", 3.8, 1.75);

    // Store them polymorphically
    vector<shared_ptr<Ride>> rides { r1, r2, r3, r4 };

    cout << "=== Polymorphism demo: mixed Ride list ===\n";
    for (const auto& ride : rides) {
        // Virtual dispatch calls the right fare() and rideType() per object
        ride->rideDetails(cout);
        cout << "\n";
    }
    cout << "\n";

    // Driver and Rider flows
    Driver d1("D01", "Avery", 4.88);
    for (const auto& ride : rides) d1.addRide(ride);
    d1.getDriverInfo(cout);
    cout << "\n";

    Rider u1("U01", "Sebastian");
    for (const auto& ride : rides) u1.requestRide(ride);
    u1.viewRides(cout);
    cout << "\n";
