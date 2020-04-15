#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <queue>
#include <iostream>
#include <stdio.h>
#include <algorithm>


using namespace std;

int completed;
struct Process {
	int PID, queue_num, arrival_time, cycles_num;
	vector<int> BTs;
};

int timer;

int last_arrived;
vector<int> IOs; // 0: queue_num, 1:PID, 2:time_left;


vector<int> now_burst; // process의 현재 진행 상황. index = pid, now burst num.
vector<Process> processes;
vector<deque<Process>> q (4);
queue<vector<int>> will_wake_up;

struct comp {
	bool operator()(Process a, Process b) {
		return a.BTs[now_burst[a.PID]] > b.BTs[now_burst[b.PID]];
	}
};

priority_queue<Process, vector<Process>, comp > q2;
vector<vector<int>> sorted_IO;
vector<int> ended_time, cpu_io_sum;
int time, pn;
//sort by left time
bool sort_comp(vector<int> a, vector<int> b) {
	return a[2] > b[2];
}



void push_IO(int queue_num, int pid, int time_left) {
	sorted_IO.push_back({queue_num, pid, time_left, 1 });
}

void print_q2() {
	if (!q2.empty())
		cout << "q2 now top: " << q2.top().PID << endl;
}
void print_processes() {
	cout << endl;
	for (int i = 0; i < processes.size(); i++) {
		cout << "process num: " << i << endl;
		for (int j = 0; j < processes[i].BTs.size(); j++) {
			cout << processes[i].BTs[j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

pair<bool, int> q_zero_and_one(int qn, int time_quantom, int time) {

	Process now_p = q[qn].front();
	int pid = now_p.PID;
	int now_burst_num = now_burst[pid];


	//CPU 면 bt 1 감소
	if ((now_burst_num+2) % 2 == 0) { // 짝수: CPU burst time
		//현재 프로세스의 현재 세고있는 burst time 감소시킴 
		q[qn].front().BTs[now_burst_num]--;
		processes[pid].BTs[now_burst_num]--;
	}
	else {
		//딱 처음 IO 시작할 타이밍이라면 sleep 시키고 넘김
		push_IO(qn, pid, now_p.BTs[now_burst[pid]]);
		q[qn].pop_front();
		return { false, pid }; //넘기고 끝내야함.
	}

	//time_quantom 다 썼으면 프로세스를 다음 큐로 넘기기
	if (time_quantom == 0) {
		//cout << "time quantom out! " << endl;
		//time quantom도 다 쓰고, 시간도 딱 다 썼으면 번호 + 해줘야 함
		if (processes[pid].BTs[now_burst_num] == 0) {
			now_burst[pid]++;
			//딱 맞게 프로세스 종료되는 케이스
			if (now_burst[pid] == now_p.BTs.size()) {
				q[qn].pop_front();
				completed++;
				ended_time[pid] = time;
			}
		
		}
		if (qn == 0) q[qn + 1].push_back(processes[pid]);
		else if (qn == 1) q2.push(processes[pid]);
		
		q[qn].pop_front();
		return  { false, pid }; //q1작업 끝
	}

	//CPU burst 다 쓰면 IO burst로 넘어가기
	else if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;
			
		//아예 프로세스의 끝이면 종료하기
		if (now_burst[pid] == now_p.BTs.size()) {
			cout << "in q"<< qn<<": process #" << pid << " is ended." << endl;
			q[qn].pop_front();
			completed++;
			ended_time[pid] = time;
		}
		//끝이 아니면 IO로 넘어감
		else {
			printf("(process #%d switched to IO mode at %d (in queue #%d))", pid, time, qn );
			push_IO(qn, pid, processes[pid].BTs[now_burst[pid]] );
			q[qn].pop_front();
		}
		return  { false, pid }; //q1작업 끝
	}
	return  { true, pid };
}
void print_ios() {
	for (auto io : sorted_IO) {
		cout << "io.pid: " << io[1] << ", left time: " << processes[io[1]].BTs[now_burst[io[1]]] <<" ";
	}
}
//1초에 한번씩 실행할 함수.
void IO() {
	//print_ios();
	for (auto io : sorted_IO) {
		io[2]--;
		processes[io[1]].BTs[now_burst[io[1]]]--;
	}
	for (int i = sorted_IO.size() - 1; i >= 0; i--) {
		//0이라서 wakeup 해야하면? will wake up 에 넣어줌 // 1 upper queue에 넣어줌
		if (processes[sorted_IO[i][1]].BTs[now_burst[sorted_IO[i][1]]] == 0) {
			if (sorted_IO[i][0] > 0) {
				sorted_IO[i][0]--;
			}
			will_wake_up.push(sorted_IO[i]);
			//cout << "pushed " << sorted_IO[i][1] << " to will wake up" << endl;
			sorted_IO.erase(sorted_IO.begin()+i);
		}
	}

}

void wake_up(int time) {
	while (!will_wake_up.empty()) {
		vector<int> t = will_wake_up.front();
		will_wake_up.pop();
		//io로 깨어난 애면 burst 번호 올려줌
		if (t[3]) {
			cout << "(process #" << t[1] << " ended IO and woke up.)" << endl;
			now_burst[t[1]]++;
		}
		if (t[0] != 2) q[t[0]].push_back(processes[t[1]]);
		else { // preemtion이 일어나야 함
			if (q2.empty()) q2.push(processes[t[1]]);
			else {
				int q2_pn = q2.top().PID;
				cout << "신인: " << processes[t[1]].BTs[now_burst[t[1]]] << ", 기존: " << processes[q2.top().PID].BTs[now_burst[q2_pn]] << endl;
				if (processes[t[1]].BTs[now_burst[t[1]]] < processes[q2.top().PID].BTs[now_burst[q2_pn]]) { //지금 새로 깨운 애가 원래 하고있던 q2 프로세스보다 짧게 남았으면 preemtion이 일어남.
					cout << "process #" << t[1] << " preempted process #" << q2_pn << endl;
					q[3].push_back(q2.top());
					q2.pop();
					q2.push(processes[t[1]]);
					printf("%d~%d | process #%d (in queue #2)\n", time - timer, time, t[1]);
					timer = 0;
				}
				else { //preemtion 일어나지 않는 경우
					q2.push(processes[t[1]]);
				}
			}
			
		}
	}
}


void arrive(int now_time) {
	for (int i = last_arrived; i < processes.size(); i++) {
		if (processes[i].arrival_time == 0) continue;
		if (processes[i].arrival_time+1 == now_time) {
			cout << "process #" << i << " arrived at " << now_time-1 << endl;
			int qn = processes[i].queue_num;
			will_wake_up.push({ qn, i, -1,0 });
			last_arrived = i;
		}
	}
}
pair<bool, int> q_two(int time) {
	if (q2.empty()) {
		cout << "q2 is empty" << endl;
		return { false, -3 };
	}
	Process now_p = q2.top();
	int pid = now_p.PID;
	int now_burst_num = now_burst[pid];


	//CPU 면 bt 1 감소
	if ((now_burst_num + 2) % 2 != 0) cout << "q2 burst num is odd. something wrong" << endl;
	//현재 프로세스의 현재 세고있는 burst time 감소시킴
	
	now_p.BTs[now_burst_num] -=1;
	processes[pid].BTs[now_burst_num] -= 1;

	//CPU burst 다 쓰면 IO burst로 넘어가기
	if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;

		//아예 프로세스의 끝이면 종료하기
		if (now_burst[pid] == processes[pid].BTs.size()) {
			printf("(process #%d is ended at %d (in queue #2)\n", pid, time);
			q2.pop();
			completed++;
			ended_time[pid] = time;
		}
		//끝이 아니면 IO로 넘어감
		else {
			printf("(process #%d switched to IO mode at %d (in queue #2))\n", pid, time);
			push_IO(2, pid, processes[pid].BTs[now_burst[pid]]);
			q2.pop();
		}
		return { false, pid }; // q2 작업 끝나면 return false
	}
	return { true, pid };
}

pair<bool, int> q_three(int time) {
	Process now_p = q[3].front();
	int pid = now_p.PID;
	//cout << "q3 now pid: " << pid << endl;
	int now_burst_num = now_burst[pid];

	//CPU 면 bt 1 감소
	if ((now_burst_num + 2) % 2 != 0) cout << "q3 burst num is odd. something wrong" << endl;
	//현재 프로세스의 현재 세고있는 burst time 감소시킴 
	q[3].front().BTs[now_burst_num] -= 1;
	processes[pid].BTs[now_burst_num] -= 1;
	if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;

		//아예 프로세스의 끝이면 종료하기
		if (now_burst[pid] == processes[pid].BTs.size()) {
			printf("(process #%d is ended at %d (in queue #3)\n", pid, time);
			q[3].pop_front();
			completed++;
			ended_time[pid] = time;
		}
		//끝이 아니면 IO로 넘어감
		else {
			cout << "process " << pid << " switched to IO mode " << endl;
			push_IO(0, pid, now_p.BTs[now_burst[pid]]);
			q[3].pop_front();
		}
		return {false,pid}; //q3 작업 끝
	}
	return { true,pid };

}
int main() {

	FILE* fp = NULL;
	fp = fopen("input2.txt", "r");
	while (!feof(fp)) {
		fscanf(fp, "%d\n", &pn);
		ended_time.resize(pn + 1, 0);
		now_burst.resize(pn+1, 0);
		for (int i = 0; i < pn; i++) {
			Process pp; int qn;
			pp.PID = i;
			fscanf(fp, "%d %d %d ", &pp.queue_num, &pp.arrival_time, &pp.cycles_num);
			for (int j = 0; j < pp.cycles_num * 2 - 1; j++) {
				int tmp=0;
				fscanf(fp, "%d ", &tmp);
				pp.BTs.push_back(tmp);
			}
			if (pp.arrival_time == 0) {
				if ((qn = pp.queue_num) != 2)
					q[qn].push_back(pp);
				else q2.push(pp);
				cout << "process #" << i << " arrived at " << 0 << endl;
			}
			processes.push_back(pp);
		}
	}
	

	for (int i = 0; i < processes.size(); i++) {
		int tmp = 0;
		for (auto b : processes[i].BTs) {
			tmp += b;
		}
		cpu_io_sum.push_back(tmp);
	}

	int time = 0;
	pair<bool, int> result;
	while (completed < processes.size()) {
		if (!q[0].empty()) {	
			int tmp = 2;
			timer = 0;
			
			while (tmp--) {
			
				timer++;
				arrive(time++);

				//깨우기
				wake_up(time);
				//IO 1감소& 0되면넘김
				IO();
				result = q_zero_and_one(0, tmp, time);
				if (!result.first) break;
			}

			printf("%d~%d | process #%d (in queue #0)\n", time - timer, time, result.second);
		}
		else if (!q[1].empty()) {
		
			timer = 0;
			int tmp = 6;
			while (tmp--) {
				timer++;
				arrive(time++);
				//print_processes();
				//깨우기
				wake_up(time);
				//IO 1감소& 0되면넘김
				IO();
				result = q_zero_and_one(1, tmp, time);
				if (!result.first) break;
			}
			printf("%d~%d | process #%d (in queue #1)\n", time - timer, time, result.second);
		}
		else if (!q2.empty()) {
		
			timer = 0;
			while (1) {
				timer++;
				arrive(time++);
				//print_processes();
				//깨우기
				wake_up(time);
				//IO 1감소& 0되면넘김
				IO();
				result = q_two(time);
				if (!result.first) break;
			};
			if (result.second == -3) cout << "preemption occured at " << time << endl;
			else printf("%d~%d | process #%d (in queue #2)\n", time - timer, time, result.second);

		}
		else if (!q[3].empty()) {
	
			timer = 0;
			while (1) {
				timer++;
				arrive(time++);
				//print_processes();
				//깨우기
				wake_up(time); //preemption 포함임
				//IO 1감소& 0되면넘김
				IO();
				result = q_three(time);
				if (!result.first) break;
			};
			printf("%d~%d | process #%d (in queue #3)\n", time - timer, time, result.second);
		}
		else {
			arrive(time++);
			//cout << " only IO working! " << endl;
			wake_up(time);
			//print_processes();
			IO();
		}
	}
	int tt_sum = 0, wt_sum = 0;
	cout << endl;
	for (int i = 0; i < processes.size(); i++) {
		int tt = ended_time[i] - processes[i].arrival_time;
		int wt = tt - cpu_io_sum[i];
		tt_sum += tt;
		wt_sum += wt;
		cout << "process #" << i << " | TT: " << tt << ", WT: " << wt << endl;
	}
	cout << "\navg of TTs: " << tt_sum / pn << ", avg of WTs: " << wt_sum / pn << endl;
	return 0;
}