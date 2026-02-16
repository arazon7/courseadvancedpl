Encapsulation
- Driver keeps assignedRides private; interact via addRide:, totalEarnings, assignedRidesCopy, getDriverInfo, assignedRideCount.
- Rider keeps requestedRides private; interact via requestRide:, viewRides, requestedRideCount.
- Ride state is accessed through methods; no external direct mutation of instance variables.

Inheritance
- StandardRide and PremiumRide are subclasses of Ride and share the Ride API.

Polymorphism
- fare and rideType are abstract (subclassResponsibility) in Ride and implemented in subclasses.
- A heterogeneous collection { r1. r2. r3. r4 } is iterated; sending rideDetails uses each subclass's fare/rideType.
"

Object subclass: #Ride
    instanceVariableNames: 'rideID pickupLocation dropoffLocation distance'
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!Ride methodsFor: 'initialization'!
initializeRideID: id pickup: pick dropoff: drop distance: dist
    rideID := id.
    pickupLocation := pick.
    dropoffLocation := drop.
    distance := dist.
    ^ self
!
! !

!Ride methodsFor: 'accessing'!
rideID ^ rideID !
pickupLocation ^ pickupLocation !
dropoffLocation ^ dropoffLocation !
distance ^ distance !
! !

!Ride methodsFor: 'behavior'!
fare
    ^ self subclassResponsibility
!
rideType
    ^ self subclassResponsibility
!
rideDetails
    "Format with integers only to avoid float printing edge-cases."
    | fareCents dollars cents fareStr distanceTenths whole tenths distStr |
    fareCents := (self fare * 100) rounded asInteger.
    dollars := fareCents // 100.
    cents := fareCents \\ 100.
    fareStr := dollars printString , '.' ,
        (cents < 10 ifTrue: ['0', cents printString] ifFalse: [cents printString]).

    distanceTenths := (self distance * 10) rounded asInteger.
    whole := distanceTenths // 10.
    tenths := distanceTenths \\ 10.
    distStr := whole printString , '.' , tenths printString.

    ^ String streamContents: [:s |
        s nextPut: $[; nextPutAll: self rideType; nextPutAll: '] '.
        s nextPutAll: rideID; nextPutAll: ' | '.
        s nextPutAll: pickupLocation; nextPutAll: ' -> '; nextPutAll: dropoffLocation.
        s nextPutAll: ' | '; nextPutAll: distStr; nextPutAll: ' mi'.
        s nextPutAll: ' | fare: $'; nextPutAll: fareStr ]
!
! !


"----- Derived classes (Inheritance) -----"

Ride subclass: #StandardRide
    instanceVariableNames: ''
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!StandardRide methodsFor: 'behavior'!
rideType ^ 'Standard' !
fare ^ 3 + (self distance * 1.5) !
! !


Ride subclass: #PremiumRide
    instanceVariableNames: 'multiplier'
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!PremiumRide methodsFor: 'initialization'!
initializeRideID: id pickup: pick dropoff: drop distance: dist surge: mult
    multiplier := mult.
    ^ super initializeRideID: id pickup: pick dropoff: drop distance: dist
!
! !

!PremiumRide methodsFor: 'behavior'!
rideType ^ 'Premium' !
fare ^ 5 + (self distance * 2.5 * multiplier) !
! !


"----- Driver (Encapsulation) -----"
Object subclass: #Driver
    instanceVariableNames: 'driverID name rating assignedRides'
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!Driver methodsFor: 'initialization'!
initializeDriverID: id name: aName rating: aRating
    driverID := id.
    name := aName.
    rating := aRating.
    assignedRides := OrderedCollection new.
    ^ self
!
! !

!Driver methodsFor: 'behavior'!
addRide: aRide
    assignedRides add: aRide
!
assignedRidesCopy
    ^ assignedRides copy
!
assignedRideCount
    ^ assignedRides size
!
totalEarnings
    ^ assignedRides inject: 0 into: [:sum :r | sum + r fare]
!
getDriverInfo
    "Return formatted summary; avoid float printing directly."
    | ratingHundredths rWhole rCents ratingStr totalCents tdollars tcents |
    ratingHundredths := (rating * 100) rounded asInteger.
    rWhole := ratingHundredths // 100.
    rCents := ratingHundredths \\ 100.
    ratingStr := rWhole printString , '.' ,
        (rCents < 10 ifTrue: ['0', rCents printString] ifFalse: [rCents printString]).

    totalCents := ((self totalEarnings) * 100) rounded asInteger.
    tdollars := totalCents // 100.
    tcents := totalCents \\ 100.

    ^ String streamContents: [:s |
        s nextPutAll: 'Driver '; nextPutAll: name; nextPutAll: ' ('; nextPutAll: driverID; nextPutAll: ')'.
        s nextPutAll: ' | rating: '; nextPutAll: ratingStr; cr.
        s nextPutAll: 'Assigned rides:'; cr.
        assignedRides do: [:r | s nextPutAll: '  - '; nextPutAll: r rideDetails; cr ].
        s nextPutAll: 'Total earnings: $'.
        s nextPutAll: tdollars printString; nextPut: $.;
           nextPutAll: (tcents < 10 ifTrue: ['0', tcents printString] ifFalse: [tcents printString]) ]
!
! !


"----- Rider (Encapsulation) -----"
Object subclass: #Rider
    instanceVariableNames: 'riderID name requestedRides'
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!Rider methodsFor: 'initialization'!
initializeRiderID: id name: aName
    riderID := id.
    name := aName.
    requestedRides := OrderedCollection new.
    ^ self
!
! !

!Rider methodsFor: 'behavior'!
requestRide: aRide
    requestedRides add: aRide
!
requestedRideCount
    ^ requestedRides size
!
viewRides
    ^ String streamContents: [:s |
        s nextPutAll: 'Rider '; nextPutAll: name; nextPutAll: ' ('; nextPutAll: riderID; nextPutAll: ') ride history:'; cr.
        requestedRides do: [:r | s nextPutAll: '  - '; nextPutAll: r rideDetails; cr ] ]
!
! !


"----- Demo / Evidence -----"
Object subclass: #RideShareDemo
    instanceVariableNames: ''
    classVariableNames: ''
    poolDictionaries: ''
    category: 'RideShare'.

!RideShareDemo class methodsFor: 'run'!
run
    | r1 r2 r3 r4 rides d1 rd1 |
    r1 := StandardRide new
        initializeRideID: 'R1001' pickup: 'Downtown' dropoff: 'Airport' distance: 15.2.
    r2 := PremiumRide new
        initializeRideID: 'R1002' pickup: 'UT Austin' dropoff: 'The Domain' distance: 8.5 surge: 1.3.
    r3 := StandardRide new
        initializeRideID: 'R1003' pickup: 'Capitol' dropoff: 'Zilker Park' distance: 5.1.
    r4 := PremiumRide new
        initializeRideID: 'R1004' pickup: 'South Congress' dropoff: 'Downtown' distance: 3.8 surge: 1.75.

    "Polymorphism demo: mixed list of Ride references"
    rides := { r1. r2. r3. r4 }.
    Transcript nextPutAll: '=== Polymorphism demo: mixed Ride list ==='; cr.
    rides do: [:r | Transcript nextPutAll: r rideDetails; cr ].
    Transcript cr.

    "Encapsulation demo (Driver)"
    d1 := Driver new initializeDriverID: 'D01' name: 'Avery' rating: 4.88.
    rides do: [:r | d1 addRide: r ].
    Transcript nextPutAll: '[Encapsulation] Driver assigned ride count = ';
              nextPutAll: d1 assignedRideCount printString; cr.
    Transcript nextPutAll: d1 getDriverInfo; cr; cr.

    "Encapsulation demo (Rider)"
    rd1 := Rider new initializeRiderID: 'U01' name: 'Sebastian'.
    rides do: [:r | rd1 requestRide: r ].
    Transcript nextPutAll: '[Encapsulation] Rider requested ride count = ';
              nextPutAll: rd1 requestedRideCount printString; cr.
    Transcript nextPutAll: rd1 viewRides; cr.
!
! !

"Entry point"
RideShareDemo run.
