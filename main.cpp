#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <unordered_set>
#include <vector>
#include <tuple>
#include <queue>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>

using namespace std;

struct Expense {
    string payer;
    double amount{};
    // participant -> share amount (absolute currency)
    map<string,double> shares;
};

static constexpr double EPS = 1e-6;

struct Book {
    unordered_set<string> users;
    vector<Expense> expenses;

    bool hasUser(const string& u) const { return users.count(u) != 0; }
    void addUser(const string& u) { users.insert(u); }

    // Add equal-split expense
    bool addExpenseEqual(const string& payer, double amount, const vector<string>& participants, string& err){
        if (!hasUser(payer)) { err = "Unknown payer: " + payer; return false; }
        if (participants.empty()) { err = "No participants."; return false; }
        for (size_t i=0;i<participants.size();++i)
            if (!hasUser(participants[i])) { err = "Unknown participant: " + participants[i]; return false; }

        double share = amount / static_cast<double>(participants.size());
        Expense e; e.payer = payer; e.amount = amount;
        for (size_t i=0;i<participants.size();++i) e.shares[participants[i]] += share;
        expenses.push_back(e);
        return true;
    }

    // Add exact-split expense with tokens like name:amount
    bool addExpenseExact(const string& payer, double amount, const vector<string>& tokens, string& err){
        if (!hasUser(payer)) { err = "Unknown payer: " + payer; return false; }
        if (tokens.empty()) { err = "No shares provided."; return false; }
        Expense e; e.payer = payer; e.amount = amount;
        double sumShares = 0.0;
        for (size_t i=0;i<tokens.size();++i){
            const string& t = tokens[i];
            size_t pos = t.find(':');
            if (pos==string::npos) { err = "Bad token '"+t+"', expected name:amount"; return false; }
            string name = t.substr(0,pos);
            double s = stod(t.substr(pos+1));
            if (!hasUser(name)) { err = "Unknown participant: " + name; return false; }
            e.shares[name] += s;
            sumShares += s;
        }
        if (fabs(sumShares - amount) > 0.01) {
            err = "Share sum (" + to_string(sumShares) + ") != amount (" + to_string(amount) + ")";
            return false;
        }
        expenses.push_back(e);
        return true;
    }

    // Compute net for each user: +ve means others owe them
    map<string,double> computeNet() const {
        map<string,double> net;
        for (unordered_set<string>::const_iterator it = users.begin(); it != users.end(); ++it)
            net[*it] = 0.0;

        for (size_t i=0;i<expenses.size();++i){
            const Expense& e = expenses[i];
            // payer paid entire amount upfront
            net[e.payer] += e.amount;
            // each participant owes their share
            for (map<string,double>::const_iterator it = e.shares.begin(); it != e.shares.end(); ++it)
                net[it->first] -= it->second;
        }
        // clamp tiny noise to 0
        for (map<string,double>::iterator it = net.begin(); it != net.end(); ++it)
            if (fabs(it->second) < 1e-9) it->second = 0.0;
        return net;
    }

    // Min-cash-flow settlement (greedy)
    vector<tuple<string,string,double>> settle() const {
        map<string,double> net = computeNet();

        struct Node { string name; double amt; }; // amt>0 creditor; amt<0 debtor
        vector<Node> cred, debt;
        for (map<string,double>::const_iterator it = net.begin(); it != net.end(); ++it) {
            const string& u = it->first;
            double amt = it->second;
            if (amt > EPS) cred.push_back(Node{u, amt});
            else if (amt < -EPS) debt.push_back(Node{u, amt});
        }

        // priority queues (max creditor, most negative debtor)
        struct CmpCred { bool operator()(const Node& a, const Node& b) const { return a.amt < b.amt; } }; // max-heap
        struct CmpDebt { bool operator()(const Node& a, const Node& b) const { return a.amt > b.amt; } }; // min (most negative) first
        priority_queue<Node, vector<Node>, CmpCred> C(cred.begin(), cred.end());
        priority_queue<Node, vector<Node>, CmpDebt> D(debt.begin(), debt.end());

        vector<tuple<string,string,double>> txns;
        while (!C.empty() && !D.empty()){
            Node c = C.top(); C.pop();
            Node d = D.top(); D.pop();
            double pay = std::min(c.amt, -d.amt);
            if (pay > EPS) txns.push_back(make_tuple(d.name, c.name, pay));
            c.amt -= pay;
            d.amt += pay;

            if (c.amt > EPS) C.push(c);
            if (d.amt < -EPS) D.push(d);
        }
        return txns;
    }

    // Save / Load (very simple text format)
    bool save(const string& path, string& err) const {
        ofstream out(path.c_str());
        if (!out){ err="Cannot open file for writing."; return false; }
        out << "USERS " << users.size() << "\n";
        for (unordered_set<string>::const_iterator it = users.begin(); it != users.end(); ++it)
            out << *it << "\n";
        out << "EXPENSES " << expenses.size() << "\n";
        out.setf(std::ios::fixed); out << setprecision(2);
        for (size_t i=0;i<expenses.size();++i){
            const Expense& e = expenses[i];
            out << "PAYER " << e.payer << " AMT " << e.amount << "\n";
            out << "SHARES " << e.shares.size() << "\n";
            for (map<string,double>::const_iterator it=e.shares.begin(); it!=e.shares.end(); ++it)
                out << it->first << " " << it->second << "\n";
        }
        return true;
    }

    bool load(const string& path, string& err){
        ifstream in(path.c_str());
        if (!in){ err="Cannot open file for reading."; return false; }
        users.clear(); expenses.clear();

        string tag; size_t n = 0;
        if (!(in >> tag >> n) || tag!="USERS"){ err="Corrupt file (USERS)."; return false; }
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        for (size_t i=0;i<n;++i){
            string u; getline(in,u);
            if (u.empty()) { --i; continue; }
            users.insert(u);
        }

        if (!(in >> tag >> n) || tag!="EXPENSES"){ err="Corrupt file (EXPENSES)."; return false; }
        for (size_t k=0;k<n;++k){
            string tag1, tag2, payer; double amt;
            if (!(in >> tag1 >> payer >> tag2 >> amt) || tag1 != "PAYER" || tag2 != "AMT") {
                err = "Corrupt expense header.";
                return false;
            }
            Expense e; e.payer = payer; e.amount = amt;

            string tag3; size_t m = 0;
            if (!(in >> tag3 >> m) || tag3!="SHARES"){ err="Corrupt shares tag."; return false; }
            for (size_t i=0;i<m;++i){
                string name; double s;
                if (!(in >> name >> s)) { err="Corrupt share entry."; return false; }
                e.shares[name]=s;
            }
            expenses.push_back(e);
        }
        return true;
    }
};

static void printBalances(const map<string,double>& net){
    cout << "Balances (+ receive, - pay)\n";
    cout.setf(std::ios::fixed); cout << setprecision(2);
    for (map<string,double>::const_iterator it = net.begin(); it != net.end(); ++it){
        cout << "  " << setw(12) << left << it->first << " : " << (fabs(it->second)<EPS?0.0:it->second) << "\n";
    }
}

static void printTxns(const vector<tuple<string,string,double>>& txns){
    cout.setf(std::ios::fixed); cout << setprecision(2);
    if (txns.empty()){ cout << "Everyone is settled.\n"; return; }
    cout << "Settlement transactions:\n";
    for (size_t i=0;i<txns.size();++i){
        const tuple<string,string,double>& t = txns[i];
        cout << "  " << get<0>(t) << " -> " << get<1>(t) << " : " << get<2>(t) << "\n";
    }
}

static void help(){
    cout <<
R"(Commands:
  add-user <name>
  add-expense equal <payer> <amount> <p1> <p2> ...
  add-expense exact <payer> <amount> <name1:share1> <name2:share2> ...
  balances
  settle
  save <file>
  load <file>
  help
  exit
)";
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Book book;
    cout << "Splitwise-CLI (C++). Type 'help' for commands.\n";

    string line;
    while (true){
        cout << "> " << flush;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd; ss >> cmd;
        if (cmd=="exit" || cmd=="quit") break;
        else if (cmd=="help"){ help(); continue; }
        else if (cmd=="add-user"){
            string name; getline(ss, name);
            if(!name.empty() && name[0]==' ') name.erase(0,1);
            if (name.empty()){ cout << "Usage: add-user <name>\n"; continue; }
            book.addUser(name);
            cout << "Added user: " << name << "\n";
        }
        else if (cmd=="add-expense"){
            string type; ss >> type;
            if (type=="equal"){
                string payer; double amount;
                ss >> payer >> amount;
                vector<string> parts; string p;
                while (ss >> p) parts.push_back(p);
                string err;
                if (!book.addExpenseEqual(payer, amount, parts, err)) cout << "Error: " << err << "\n";
                else cout << "Added equal expense.\n";
            } else if (type=="exact"){
                string payer; double amount;
                ss >> payer >> amount;
                vector<string> tokens; string t;
                while (ss >> t) tokens.push_back(t);
                string err;
                if (!book.addExpenseExact(payer, amount, tokens, err)) cout << "Error: " << err << "\n";
                else cout << "Added exact expense.\n";
            } else {
                cout << "Usage: add-expense equal|exact ...  (see 'help')\n";
            }
        }
        else if (cmd=="balances"){
            map<string,double> net = book.computeNet();
            printBalances(net);
        }
        else if (cmd=="settle"){
            vector<tuple<string,string,double>> txns = book.settle();
            printTxns(txns);
        }
        else if (cmd=="save"){
            string file; ss >> file;
            if (file.empty()){ cout << "Usage: save <file>\n"; continue; }
            string err; 
            if (book.save(file, err)) cout << "Saved to " << file << "\n";
            else cout << "Error: " << err << "\n";
        }
        else if (cmd=="load"){
            string file; ss >> file;
            if (file.empty()){ cout << "Usage: load <file>\n"; continue; }
            string err;
            if (book.load(file, err)) cout << "Loaded from " << file << "\n";
            else cout << "Error: " << err << "\n";
        }
        else {
            cout << "Unknown command. Type 'help'.\n";
        }
    }
    cout << "Bye!\n";
    return 0;
}
