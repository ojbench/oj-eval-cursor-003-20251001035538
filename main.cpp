#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    bool solved = false;
    int solve_time = 0;
    int wrong_before_solve = 0;
    int wrong_count = 0;
    bool frozen = false;
    int frozen_count = 0;
    int wrong_before_freeze = 0;
    int first_ac_time_in_freeze = -1; // -1 means no AC during freeze
    int wrongs_before_ac_in_freeze = 0;
    int total_wrongs_in_freeze = 0;
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    int solved_count = 0;
    int penalty_time = 0;
    int ranking = 0;
    vector<int> solve_times; // for tie-breaking (unsorted)
    vector<int> sorted_solve_times; // sorted descending for comparison
    
    void update_sorted_times() {
        sorted_solve_times = solve_times;
        sort(sorted_solve_times.rbegin(), sorted_solve_times.rend());
    }
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> team_order; // for scoreboard display
    bool started = false;
    bool frozen = false;
    int duration_time = 0;
    int problem_count = 0;
    
    void update_scoreboard() {
        team_order.clear();
        for (auto& p : teams) {
            team_order.push_back(p.first);
        }
        
        stable_sort(team_order.begin(), team_order.end(), [this](const string& a, const string& b) {
            Team& ta = teams[a];
            Team& tb = teams[b];
            
            // First compare solved count
            if (ta.solved_count != tb.solved_count) {
                return ta.solved_count > tb.solved_count;
            }
            
            // Then compare penalty time
            if (ta.penalty_time != tb.penalty_time) {
                return ta.penalty_time < tb.penalty_time;
            }
            
            // Compare sorted solve times
            int min_size = min(ta.sorted_solve_times.size(), tb.sorted_solve_times.size());
            for (int i = 0; i < min_size; i++) {
                if (ta.sorted_solve_times[i] != tb.sorted_solve_times[i]) {
                    return ta.sorted_solve_times[i] < tb.sorted_solve_times[i];
                }
            }
            
            // Finally compare by name
            return a < b;
        });
        
        // Update rankings
        for (int i = 0; i < team_order.size(); i++) {
            teams[team_order[i]].ranking = i + 1;
        }
    }
    
    void print_scoreboard() {
        for (const string& team_name : team_order) {
            Team& t = teams[team_name];
            cout << team_name << " " << t.ranking << " " << t.solved_count << " " << t.penalty_time;
            
            for (int i = 0; i < problem_count; i++) {
                char prob = 'A' + i;
                cout << " ";
                
                if (t.problems.find(prob) == t.problems.end() || 
                    (!t.problems[prob].solved && t.problems[prob].wrong_count == 0 && !t.problems[prob].frozen)) {
                    cout << ".";
                } else {
                    ProblemStatus& ps = t.problems[prob];
                    if (ps.frozen) {
                        if (ps.wrong_before_freeze == 0) {
                            cout << "0/" << ps.frozen_count;
                        } else {
                            cout << "-" << ps.wrong_before_freeze << "/" << ps.frozen_count;
                        }
                    } else if (ps.solved) {
                        if (ps.wrong_before_solve == 0) {
                            cout << "+";
                        } else {
                            cout << "+" << ps.wrong_before_solve;
                        }
                    } else {
                        cout << "-" << ps.wrong_count;
                    }
                }
            }
            cout << "\n";
        }
    }
    
public:
    void add_team(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        
        Team t;
        t.name = name;
        teams[name] = t;
        cout << "[Info]Add successfully.\n";
    }
    
    void start(int dur, int prob_count) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        
        started = true;
        duration_time = dur;
        problem_count = prob_count;
        
        // Initialize rankings based on lexicographic order
        team_order.clear();
        for (auto& p : teams) {
            team_order.push_back(p.first);
        }
        sort(team_order.begin(), team_order.end());
        for (int i = 0; i < team_order.size(); i++) {
            teams[team_order[i]].ranking = i + 1;
        }
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& team_name, const string& status, int time) {
        Team& t = teams[team_name];
        char prob = problem[0];
        
        Submission sub;
        sub.problem = problem;
        sub.status = status;
        sub.time = time;
        t.submissions.push_back(sub);
        
        if (!frozen || t.problems[prob].solved) {
            // Not frozen or already solved before freeze
            if (status == "Accepted" && !t.problems[prob].solved) {
                t.problems[prob].solved = true;
                t.problems[prob].solve_time = time;
                t.problems[prob].wrong_before_solve = t.problems[prob].wrong_count;
                t.solved_count++;
                t.penalty_time += 20 * t.problems[prob].wrong_count + time;
                t.solve_times.push_back(time);
                t.update_sorted_times();
            } else if (status != "Accepted") {
                t.problems[prob].wrong_count++;
            }
        } else {
            // Frozen
            if (!t.problems[prob].solved) {
                ProblemStatus& ps = t.problems[prob];
                if (!ps.frozen) {
                    // First submission after freeze for unsolved problem
                    ps.frozen = true;
                    ps.wrong_before_freeze = ps.wrong_count;
                }
                ps.frozen_count++;
                
                if (status == "Accepted" && ps.first_ac_time_in_freeze == -1) {
                    // First AC during freeze
                    ps.first_ac_time_in_freeze = time;
                    ps.wrongs_before_ac_in_freeze = ps.total_wrongs_in_freeze;
                } else if (status != "Accepted" && ps.first_ac_time_in_freeze == -1) {
                    // Wrong submission before any AC
                    ps.total_wrongs_in_freeze++;
                }
            }
        }
    }
    
    void flush() {
        update_scoreboard();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze_scoreboard() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        
        frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // First flush
        update_scoreboard();
        print_scoreboard();
        
        // Scroll process
        while (true) {
            // Find lowest ranked team with frozen problems
            string lowest_team = "";
            int lowest_idx = -1;
            for (int i = team_order.size() - 1; i >= 0; i--) {
                Team& t = teams[team_order[i]];
                bool has_frozen = false;
                for (auto& p : t.problems) {
                    if (p.second.frozen) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen) {
                    lowest_team = team_order[i];
                    lowest_idx = i;
                    break;
                }
            }
            
            if (lowest_team.empty()) break;
            
            // Find smallest problem number that is frozen
            Team& t = teams[lowest_team];
            char smallest_prob = 'Z' + 1;
            for (auto& p : t.problems) {
                if (p.second.frozen && p.first < smallest_prob) {
                    smallest_prob = p.first;
                }
            }
            
            // Unfreeze the problem by processing frozen submissions
            ProblemStatus& ps = t.problems[smallest_prob];
            int old_rank = t.ranking;
            
            // Process frozen submissions for this problem
            bool stats_changed = false;
            if (ps.first_ac_time_in_freeze != -1) {
                // There was an AC during freeze
                ps.solved = true;
                ps.solve_time = ps.first_ac_time_in_freeze;
                ps.wrong_before_solve = ps.wrong_before_freeze + ps.wrongs_before_ac_in_freeze;
                t.solved_count++;
                t.penalty_time += 20 * (ps.wrong_before_freeze + ps.wrongs_before_ac_in_freeze) + ps.first_ac_time_in_freeze;
                t.solve_times.push_back(ps.first_ac_time_in_freeze);
                t.update_sorted_times();
                stats_changed = true;
            } else {
                // No AC during freeze, just update wrong count
                ps.wrong_count = ps.wrong_before_freeze + ps.total_wrongs_in_freeze;
            }
            
            ps.frozen = false;
            ps.frozen_count = 0;
            ps.first_ac_time_in_freeze = -1;
            ps.wrongs_before_ac_in_freeze = 0;
            ps.total_wrongs_in_freeze = 0;
            
            // Only update scoreboard if team stats changed
            
            if (stats_changed) {
                // Build simple rank map before update
                string old_leader = team_order[0];
                
                // Update scoreboard
                update_scoreboard();
                
                // Check if ranking changed (moved up)
                if (t.ranking < old_rank) {
                    // Team moved up - find who they replaced
                    // The replaced team is now at rank t.ranking + 1 (or old position)
                    string replaced_team = "";
                    for (const string& name : team_order) {
                        if (name != lowest_team && teams[name].ranking == t.ranking + 1) {
                            replaced_team = name;
                            break;
                        }
                    }
                    if (replaced_team.empty() && t.ranking < team_order.size()) {
                        replaced_team = team_order[t.ranking];
                    }
                    cout << lowest_team << " " << replaced_team << " " 
                         << t.solved_count << " " << t.penalty_time << "\n";
                }
            }
        }
        
        print_scoreboard();
        frozen = false;
    }
    
    void query_ranking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        cout << team_name << " NOW AT RANKING " << teams[team_name].ranking << "\n";
    }
    
    void query_submission(const string& team_name, const string& problem, const string& status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        Team& t = teams[team_name];
        for (int i = t.submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = t.submissions[i];
            bool match = true;
            
            if (problem != "ALL" && sub.problem != problem) {
                match = false;
            }
            
            if (status != "ALL" && sub.status != status) {
                match = false;
            }
            
            if (match) {
                cout << team_name << " " << sub.problem << " " << sub.status << " " << sub.time << "\n";
                return;
            }
        }
        
        cout << "Cannot find any submission.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.add_team(team_name);
        }
        else if (cmd == "START") {
            string dummy;
            int dur, prob_count;
            iss >> dummy >> dur >> dummy >> prob_count;
            system.start(dur, prob_count);
        }
        else if (cmd == "SUBMIT") {
            string problem, by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            system.submit(problem, team_name, status, time);
        }
        else if (cmd == "FLUSH") {
            system.flush();
        }
        else if (cmd == "FREEZE") {
            system.freeze_scoreboard();
        }
        else if (cmd == "SCROLL") {
            system.scroll();
        }
        else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.query_ranking(team_name);
        }
        else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, rest;
            iss >> team_name >> where;
            getline(iss, rest);
            
            // Parse PROBLEM=X AND STATUS=Y
            size_t prob_pos = rest.find("PROBLEM=");
            size_t and_pos = rest.find(" AND ");
            size_t stat_pos = rest.find("STATUS=");
            
            string problem = rest.substr(prob_pos + 8, and_pos - prob_pos - 8);
            string status = rest.substr(stat_pos + 7);
            
            system.query_submission(team_name, problem, status);
        }
        else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }
    
    return 0;
}
