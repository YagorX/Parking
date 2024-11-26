#pragma once

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <unordered_set>

using namespace std;
using namespace std::chrono;

class VehiclePlate {
private:
    auto AsTuple() const {
        return tie(letters_, digits_, region_);
    }

public:
    bool operator==(const VehiclePlate& other) const {
        return AsTuple() == other.AsTuple();
    }

    bool operator<(const VehiclePlate& other) const {
        return AsTuple() < other.AsTuple();
    }

    VehiclePlate(char l0, char l1, int digits, char l2, int region)
        : letters_{ l0, l1, l2 }
        , digits_(digits)
        , region_(region) {
    }

    string ToString() const {
        ostringstream out;
        out << letters_[0] << letters_[1];
        out << setfill('0') << right << setw(3) << digits_;
        out << letters_[2] << setw(2) << region_;

        return out.str();
    }

    int Hash() const {
        return digits_;
    }

    const array<char, 3>& GetLetters() const {
        return letters_;
    }

    int GetDigits() const {
        return digits_;
    }

    int GetRegion() const {
        return region_;
    }

private:
    array<char, 3> letters_;
    int digits_;
    int region_;
};

ostream& operator<<(ostream& out, VehiclePlate plate) {
    out << plate.ToString();
    return out;
}

class VehiclePlateHasher {
public:
    inline size_t operator()(const VehiclePlate& plate) const {
        std::string number_plate_str(plate.ToString());
        size_t hashed_str = std::hash<std::string>{}(number_plate_str);
        return hashed_str;
    }
};

class VehicleEfficPlateHasher {
public:
    inline size_t operator()(const VehiclePlate& plate) const {
        auto letters = plate.GetLetters();
        auto number = plate.GetDigits();
        auto region = plate.GetRegion();
        uint64_t number_plate_int = ((letters[0] - 'A') * 100000000000) + (number * 10000000) + ((letters[1] - 'A') * 100000) + ((letters[1] - 'A') * 1000) + region;
        return number_plate_int;
    }
};

class ParkingCounter {
public:
    void Park(VehiclePlate car) {
        auto obj = car_to_parks_.find(car);
        if (obj != car_to_parks_.end())
            obj->second++;
        else
            ++car_to_parks_[car];
    }

    int GetCount(const VehiclePlate& car) const {
        auto obj_ind = car_to_parks_.find(car);
        if (obj_ind == car_to_parks_.end())
            return 0;
        return obj_ind->second;
    }

    auto& GetAllData() const {
        return car_to_parks_;
    }

private:
    std::unordered_map<VehiclePlate, int, VehiclePlateHasher> car_to_parks_;
    std::hash<string> hasher;
};

class TestClock {
public:
    using time_point = chrono::system_clock::time_point;
    using duration = chrono::system_clock::duration;

    static void SetNow(int seconds) {
        current_time_ = seconds;
    }

    static time_point now() {
        return start_point_ + chrono::seconds(current_time_);
    }

private:
    inline static time_point start_point_ = chrono::system_clock::now();
    inline static int current_time_ = 0;
};


struct ParkingException {};

template <typename Clock>
class Parking {
    using Duration = typename TestClock::duration;
    using TimePoint = typename TestClock::time_point;

public:
    Parking(int cost_per_second) : cost_per_second_(cost_per_second) {}

    void Park(VehiclePlate car) {
        if (now_parked_.find(car) != now_parked_.end())
            throw ParkingException{};
        now_parked_.insert(std::pair<VehiclePlate, TimePoint>(car, Clock::now()));
    }

    void Withdraw(const VehiclePlate& car) {
        auto car_parked = now_parked_.find(car);
        if (car_parked == now_parked_.end())
            throw ParkingException{};
        TimePoint st_arend = car_parked->second;
        Duration final_time = Clock::now() - st_arend; //find time which car was on park

        now_parked_.erase(car_parked);
        complete_parks_.insert(std::pair<VehiclePlate, Duration>(car, final_time));
    }

    int64_t GetCurrentBill(const VehiclePlate& car) const {
        int64_t result{};
        auto complete_park_now = complete_parks_.find(car);
        auto now_parked = now_parked_.find(car);
        if (complete_park_now == complete_parks_.end()) {
            if (now_parked == now_parked_.end())
                return 0;
            else {
                TimePoint st_arend = now_parked->second;
                Duration time_parked = Clock::now() - st_arend;
                auto count_sec2 = chrono::duration_cast<chrono::seconds>(time_parked).count();
                return count_sec2 * cost_per_second_;
            }
        }

        if (now_parked == now_parked_.end()) {
            if (complete_park_now == complete_parks_.end())
                return 0;
            else {
                auto car_parked = complete_parks_.find(car);
                auto count_sec = chrono::duration_cast<chrono::seconds>(car_parked->second).count();
                return count_sec * cost_per_second_;
            }
        }
        else {
            if (complete_park_now != complete_parks_.end()) {
                auto car_parked = complete_parks_.find(car);
                auto count_sec = chrono::duration_cast<chrono::seconds>(car_parked->second).count();
                result = count_sec * cost_per_second_;

                TimePoint st_arend = now_parked->second;
                Duration time_parked = Clock::now() - st_arend;
                auto count_sec2 = chrono::duration_cast<chrono::seconds>(time_parked).count();
                return result + count_sec2 * cost_per_second_;
            }
        }
    }

    unordered_map<VehiclePlate, int64_t, VehiclePlateHasher> EndPeriodAndGetBills()
    {
        unordered_map<VehiclePlate, int64_t, VehiclePlateHasher> result;
        if (!complete_parks_.empty()) {
            for (auto [car, duration_time] : complete_parks_) {
                uint64_t cost_park{};
                if (now_parked_.find(car) != now_parked_.end())
                {
                    auto it_car_now_park = now_parked_.find(car);
                    Duration time_now_car_park = Clock::now() - it_car_now_park->second;
                    auto count_sec2 = chrono::duration_cast<chrono::seconds>(time_now_car_park).count();
                    it_car_now_park->second = Clock::now(); //обнуляем счётчик стоянки машины которая сейчас на парковке
                    cost_park += cost_per_second_ * count_sec2;
                }

                auto count_sec = chrono::duration_cast<chrono::seconds>(duration_time).count();
                cost_park += count_sec * cost_per_second_;
                result.insert(std::pair<VehiclePlate, uint64_t>(car, cost_park));
            }
            complete_parks_.clear();
        }
        else {
            for (auto [car, start_time] : now_parked_)
            {
                Duration time_now_park = Clock::now() - start_time;
                auto count_sec = chrono::duration_cast<chrono::seconds>(time_now_park).count();
                result.insert(std::pair<VehiclePlate, uint64_t>(car, count_sec * cost_per_second_));
            }
        }
        return result;
    }

    auto& GetNowParked() const {
        return now_parked_;
    }

    auto& GetCompleteParks() const {
        return complete_parks_;
    }

private:
    int cost_per_second_;
    unordered_map<VehiclePlate, TimePoint, VehiclePlateHasher> now_parked_;
    unordered_map<VehiclePlate, Duration, VehiclePlateHasher> complete_parks_;
};
