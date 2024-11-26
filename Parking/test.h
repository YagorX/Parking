#pragma once

#include <cassert>
#include "Parking.h"
#include "logtimer.h"


class PlateGenerator {
    char GenerateChar() {
        uniform_int_distribution<short> char_gen{ 0, static_cast<short>(possible_chars_.size() - 1) };
        return possible_chars_[char_gen(engine_)];
    }

    int GenerateNumber() {
        uniform_int_distribution<short> num_gen{ 0, 999 };
        return num_gen(engine_);
    }

    int GenerateRegion() {
        uniform_int_distribution<short> region_gen{ 0, static_cast<short>(possible_regions_.size() - 1) };
        return possible_regions_[region_gen(engine_)];
    }

public:
    VehiclePlate Generate() {
        return VehiclePlate(GenerateChar(), GenerateChar(), GenerateNumber(), GenerateChar(), GenerateRegion());
    }

private:
    mt19937 engine_;

    inline static const array possible_regions_
        = { 1,  2,  102, 3,   4,   5,   6,   7,   8,  9,   10,  11,  12, 13,  113, 14,  15, 16,  116, 17, 18,
           19, 20, 21,  121, 22,  23,  93,  123, 24, 84,  88,  124, 25, 125, 26,  27,  28, 29,  30,  31, 32,
           33, 34, 35,  36,  136, 37,  38,  85,  39, 91,  40,  41,  82, 42,  142, 43,  44, 45,  46,  47, 48,
           49, 50, 90,  150, 190, 51,  52,  152, 53, 54,  154, 55,  56, 57,  58,  59,  81, 159, 60,  61, 161,
           62, 63, 163, 64,  164, 65,  66,  96,  67, 68,  69,  70,  71, 72,  73,  173, 74, 174, 75,  80, 76,
           77, 97, 99,  177, 199, 197, 777, 78,  98, 178, 79,  83,  86, 87,  89,  94,  95 };

    inline static const string_view possible_chars_ = "ABCEHKMNOPTXY"sv;
};

void Test_FirstStep() {
    ParkingCounter parking;

    parking.Park({ 'B', 'H', 840, 'E', 99 });
    parking.Park({ 'O', 'K', 942, 'K', 78 });
    parking.Park({ 'O', 'K', 942, 'K', 78 });
    parking.Park({ 'O', 'K', 942, 'K', 78 });
    parking.Park({ 'O', 'K', 942, 'K', 78 });
    parking.Park({ 'H', 'E', 968, 'C', 79 });
    parking.Park({ 'T', 'A', 326, 'X', 83 });
    parking.Park({ 'H', 'H', 831, 'P', 116 });
    parking.Park({ 'A', 'P', 831, 'Y', 99 });
    parking.Park({ 'P', 'M', 884, 'K', 23 });
    parking.Park({ 'O', 'C', 34, 'P', 24 });
    parking.Park({ 'M', 'Y', 831, 'M', 43 });
    parking.Park({ 'B', 'P', 831, 'M', 79 });
    parking.Park({ 'O', 'K', 942, 'K', 78 });
    parking.Park({ 'K', 'T', 478, 'P', 49 });
    parking.Park({ 'X', 'P', 850, 'A', 50 });

    assert(parking.GetCount({ 'O', 'K', 942, 'K', 78 }) == 5);
    assert(parking.GetCount({ 'A', 'B', 111, 'C', 99 }) == 0);
    for (const auto& [plate, count] : parking.GetAllData()) {
        cout << plate << " "s << count << endl;
    }
}

void TestOnClock() {
    Parking<TestClock> parking(10);

    TestClock::SetNow(10);
    parking.Park({ 'A', 'A', 111, 'A', 99 });

    TestClock::SetNow(20);
    parking.Withdraw({ 'A', 'A', 111, 'A', 99 });
    parking.Park({ 'B', 'B', 222, 'B', 99 });

    TestClock::SetNow(40);
    assert(parking.GetCurrentBill({ 'A', 'A', 111, 'A', 99 }) == 100);
    assert(parking.GetCurrentBill({ 'B', 'B', 222, 'B', 99 }) == 200);
    parking.Park({ 'A', 'A', 111, 'A', 99 });

    TestClock::SetNow(50);
    assert(parking.GetCurrentBill({ 'A', 'A', 111, 'A', 99 }) == 200);
    assert(parking.GetCurrentBill({ 'B', 'B', 222, 'B', 99 }) == 300);
    assert(parking.GetCurrentBill({ 'C', 'C', 333, 'C', 99 }) == 0);
    parking.Withdraw({ 'B', 'B', 222, 'B', 99 });

    TestClock::SetNow(70);
    {
        auto bill = parking.EndPeriodAndGetBills();

        assert((bill
            == unordered_map<VehiclePlate, int64_t, VehiclePlateHasher>{
                {{'A', 'A', 111, 'A', 99}, 400},
                { {'B', 'B', 222, 'B', 99}, 300 },
        }));
    }

    TestClock::SetNow(80);
    {
        auto bill = parking.EndPeriodAndGetBills();
        assert((bill
            == unordered_map<VehiclePlate, int64_t, VehiclePlateHasher>{
                {{'A', 'A', 111, 'A', 99}, 100},
        }));
    }

    try {
        parking.Park({ 'A', 'A', 111, 'A', 99 });
        assert(false);
    }
    catch (ParkingException) {
    }

    try {
        parking.Withdraw({ 'B', 'B', 222, 'B', 99 });
        assert(false);
    }
    catch (ParkingException) {
    }

    cout << "Success!"s << endl;
}

void Test_Hasher_Effic() {
    static const int N = 1'000'000;

    PlateGenerator generator;
    vector<VehiclePlate> fill_vector;
    vector<VehiclePlate> find_vector;

    generate_n(back_inserter(fill_vector), N, [&]() {
        return generator.Generate();
        });
    generate_n(back_inserter(find_vector), N, [&]() {
        return generator.Generate();
        });

    uint64_t found;
    {
        LOG_DURATION("unordered_set");
        unordered_set<VehiclePlate, VehicleEfficPlateHasher> container;
        for (auto& p : fill_vector) {
            container.insert(p);
        }
        found = count_if(find_vector.begin(), find_vector.end(), [&](const VehiclePlate& plate) {
            return container.count(plate) > 0;
            });
    }
    cout << "Found matches (1): "s << found << endl;

    {
        LOG_DURATION("set");
        set<VehiclePlate> container2;

        for (auto& p : fill_vector) {
            container2.insert(p);
        }

        found = count_if(find_vector.begin(), find_vector.end(), [&](const VehiclePlate& plate) {
            return static_cast<int>(container2.count(plate)) > 0;
            });
    }
    cout << "Found matches (2): "s << found << endl;

}