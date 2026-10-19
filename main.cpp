//Abdul Basit Azizi
// CS 210
// Fall 2026 - SDSU
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
using namespace std;

// professor's educational hash function.
uint64_t demoHash(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) h = (h ^ c) * 1099511628211ULL;
    return h;
}

// Store usernames, salts, and password hashes (not passwords).
struct UserRecord { string username, salt; uint64_t passwordHash; };

// four collision-handling methods.
enum class Strategy { Chaining, Linear, Quadratic, DoubleHashing };
struct Slot { UserRecord user; bool occupied = false, deleted = false; };

//  a custom hash table without unordered_map.
class HashTable {
    vector<vector<UserRecord>> buckets;
    vector<Slot> slots;
    Strategy strategy;
    size_t capacity, count = 0, collisions = 0, rehashes = 0;

    bool isPrime(size_t n) const {
        if (n < 2) return false;
        for (size_t i = 2; i <= n / i; ++i) if (n % i == 0) return false;
        return true;
    }
    size_t nextPrime(size_t n) const { while (!isPrime(n)) ++n; return n; }
    size_t hashIndex(const string& name) const { return demoHash(name) % capacity; }

    // Linear: +i; quadratic: +i*i; double hashing: +i*secondHash.
    size_t probeIndex(const string& name, size_t i) const {
        size_t start = hashIndex(name);
        if (strategy == Strategy::Linear) return (start + i) % capacity;
        if (strategy == Strategy::Quadratic) return (start + i * i) % capacity;
        return (start + i * (1 + demoHash("step:" + name) % (capacity - 1))) % capacity;
    }
    void resetStorage() {
        buckets.clear(); slots.clear();
        if (strategy == Strategy::Chaining) buckets.resize(capacity);
        else slots.resize(capacity);
    }

    // Put a user into a chain or find a free probing slot.
    // Count a collision when insertion encounters an occupied home/slot.
    bool place(const UserRecord& user, bool trackCollisions = true) {
        if (strategy == Strategy::Chaining) {
            auto& bucket = buckets[hashIndex(user.username)];
            if (trackCollisions && !bucket.empty()) ++collisions;
            bucket.push_back(user); ++count; return true;
        }
        for (size_t i = 0; i < capacity; ++i) {
            Slot& slot = slots[probeIndex(user.username, i)];
            if (!slot.occupied) {
                slot.user = user; slot.occupied = true; slot.deleted = false;
                ++count; return true;
            }
            if (trackCollisions) ++collisions;
        }
        return false;
    }

    // Automatically resize and rehash when the table gets full.
    void rehash() {
        vector<UserRecord> users;
        if (strategy == Strategy::Chaining) {
            for (const auto& bucket : buckets)
                for (const auto& user : bucket) users.push_back(user);
        } else {
            for (const auto& slot : slots) if (slot.occupied) users.push_back(slot.user);
        }
        size_t old = capacity;
        capacity = nextPrime(capacity * 2 + 1);
        count = 0; resetStorage();
        for (const auto& user : users) place(user, false);
        ++rehashes;
        cout << "  [rehash] capacity " << old << " -> " << capacity << '\n';
    }
public:
    HashTable(Strategy s, size_t initial = 7) : strategy(s), capacity(nextPrime(initial < 3 ? 3 : initial)) { resetStorage(); }

    // Implement loadFactor(), find(), insert(), and remove().
    double loadFactor() const { return double(count) / capacity; }
    const UserRecord* find(const string& name) const {
        if (strategy == Strategy::Chaining) {
            for (const auto& user : buckets[hashIndex(name)])
                if (user.username == name) return &user;
        } else {
            for (size_t i = 0; i < capacity; ++i) {
                const Slot& slot = slots[probeIndex(name, i)];
                if (!slot.occupied && !slot.deleted) break;
                if (slot.occupied && slot.user.username == name) return &slot.user;
            }
        }
        return nullptr;
    }
    bool insert(const UserRecord& user) {
        if (find(user.username)) return false;
        double limit = strategy == Strategy::Chaining ? 0.75 : 0.49;
        if (double(count + 1) / capacity > limit) rehash();
        while (!place(user)) rehash();
        return true;
    }
    bool remove(const string& name) {
        if (strategy == Strategy::Chaining) {
            auto& bucket = buckets[hashIndex(name)];
            for (size_t i = 0; i < bucket.size(); ++i) {
                if (bucket[i].username == name) {
                    bucket.erase(bucket.begin() + i); --count; return true;
                }
            }
        } else {
            for (size_t i = 0; i < capacity; ++i) {
                Slot& slot = slots[probeIndex(name, i)];
                if (!slot.occupied && !slot.deleted) break;
                if (slot.occupied && slot.user.username == name) {
                    slot.occupied = false; slot.deleted = true; --count; return true;
                }
            }
        }
        return false;
    }
    // detailed output.
    void printStats() const {
        cout << "Users: " << count << '\n';
        cout << "Capacity: " << capacity << '\n';
        cout << "Load factor: " << loadFactor() << '\n';
        cout << "Collisions: " << collisions << '\n';
        cout << "Rehashes: " << rehashes << '\n';
    }
};

// Store demoHash(salt + password + pepper).
UserRecord createUser(const string& name, const string& password, const string& salt, const string& pepper) {
    return {name, salt, demoHash(salt + password + pepper)};
}

// Verify username and password during login.
bool login(const HashTable& table, const string& name, const string& password, const string& pepper) {
    const UserRecord* user = table.find(name);
    if (!user) { cout << "Login failed for " << name << ": unknown user\n"; return false; }
    if (demoHash(user->salt + password + pepper) != user->passwordHash) {
        cout << "Login failed for " << name << ": wrong password\n"; return false;
    }
    cout << "Login succeeded for " << name << '\n';
    return true;
}

int main() {
    const string pepper = "ClassDemoPepper2026";

    //  Register at least five users, each with a different salt.
    vector<UserRecord> users = {
        createUser("Luis", "Apple123", "salt_L1", pepper),
        createUser("Carlos", "Banana456", "salt_C2", pepper),
        createUser("Noah", "Cherry789", "salt_N3", pepper),
        createUser("Jamie", "Orange321", "salt_J4", pepper),
        createUser("Bailey", "Grape654", "salt_B5", pepper),
        createUser("Taylor", "Mango987", "salt_T6", pepper)
    };

    // show all four strategies, rehashing, and login cases.
    const char* names[] = {"chaining", "linear probing", "quadratic probing", "double hashing"};
    for (int s = 0; s < 4; ++s) {
        cout << "\n===== " << names[s] << " =====\n";
        HashTable table(static_cast<Strategy>(s));
        for (const auto& user : users) {
            if (table.insert(user)) cout << "Registered " << user.username << '\n';
        }
        login(table, "Luis", "Apple123", pepper);         // Successful login
        login(table, "Carlos", "Wrong123", pepper);      // Wrong password
        login(table, "Unknown", "Hello123", pepper);     // Unknown username
        if (table.remove("Noah")) cout << "Removed Noah\n"; // Remove a user
        login(table, "Noah", "Cherry789", pepper);       // Login after removal
        table.printStats();                                // Load, collisions, rehashes
    }

    // Show two usernames that hash to the same bucket.
    cout << "\n===== Collision demonstrations =====\n";
    const string first = "Collision0", second = "Collision9";
    for (int s = 0; s < 4; ++s) {
        cout << "\n" << names[s] << ": " << first << " and " << second
             << " both hash to bucket " << demoHash(first) % 7 << '\n';
        HashTable table(static_cast<Strategy>(s));

        // Test how each collision-handling method stores both users.
        table.insert(createUser(first, "Pass123", "salt_1", pepper));
        table.insert(createUser(second, "Pass456", "salt_2", pepper));
        table.printStats();
        cout << "Find second user: " << (table.find(second) ? "Found" : "Not found") << '\n';

        // Check that the second user can still be found after removing the first.
        table.remove(first);
        cout << "Find second after removing first: " << (table.find(second) ? "Found" : "Not found") << '\n';
    }
}
