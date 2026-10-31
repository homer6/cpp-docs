#include <vector>

struct Job { int id; };

int run_jobs(int n) {
    std::vector<Job*> jobs;
    for (int i = 0; i < n; ++i) jobs.push_back(new Job{i});
    int sum = 0;
    for (Job* j : jobs) sum += j->id;
    for (Job* j : jobs) delete j;
    return sum;
}
