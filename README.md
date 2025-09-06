Account Balancing App – C++

A command-line application to manage shared expenses and compute optimal settlements.
Inspired by real travel needs and LeetCode 465 – Optimal Account Balancing
.

✨ Features

Add users and record expenses with equal or exact splits

Calculate per-user balances (+ve → receive, –ve → pay)

Generate minimal settlement transactions using a greedy algorithm with priority queues

Save/Load books to a text file for persistence

Modular design, extensible for future mobile app (Flutter/Dart)

🛠️ Tech Stack

C++17 (STL, maps, sets, priority queues, file I/O)

CLI interface (cross-platform)

Algorithms: maps, heaps, greedy cash-flow settlement

🚀 Getting Started
1. Clone the repository
git clone https://github.com/<your-username>/account-balancing-app.git
cd account-balancing-app

2. Build

Make sure you have g++ (C++17 or higher) installed.

mkdir build
g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp -o build/account_balancing

3. Run
./build/account_balancing


On Windows:

.\build\account_balancing.exe

📖 Usage (Commands)
add-user <name>
add-expense equal <payer> <amount> <p1> <p2> ...
add-expense exact <payer> <amount> <name1:share1> <name2:share2> ...
balances
settle
save <file>
load <file>
help
exit

Example
> add-user Alice
> add-user Bob
> add-user Carol
> add-expense equal Alice 300 Alice Bob Carol
> add-expense exact Bob 150 Alice:50 Bob:50 Carol:50
> balances
> settle

📂 Save/Load Format

Users and expenses are stored in a plain text format

Example saved file:

USERS 3
Alice
Bob
Carol
EXPENSES 1
PAYER Alice AMT 300.00
SHARES 3
Alice 100.00
Bob 100.00
Carol 100.00

🤝 Contribution

Contributions are welcome! Feel free to fork this repo, submit issues, or open pull requests.

📜 License

This project is released under the MIT License.
