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



vector<int> IOs; // 0: queue_num, 1:PID, 2:time_left;


vector<int> now_burst; // process�� ���� ���� ��Ȳ. index = pid, now burst num.
vector<bool> is_sleeping;
vector<Process> processes;
vector<deque<Process>> q (4);
queue<vector<int>> will_wake_up;


struct comp {
	bool operator()(Process a, Process b) {
		cout << "a--- pid: " << a.PID << " bts: " << a.BTs[now_burst[a.PID]] << ", b--- pid:" << b.PID << " bts: " << b.BTs[now_burst[b.PID]] << endl;
		return a.BTs[now_burst[a.PID]] > b.BTs[now_burst[b.PID]];
	}
};

priority_queue<Process, vector<Process>, comp > q2;
vector<vector<int>> sorted_IO;

int time, pn;
//sort by left time
bool sort_comp(vector<int> a, vector<int> b) {
	return a[2] > b[2];
}



void push_IO(int queue_num, int pid, int time_left) {
	sorted_IO.push_back({queue_num, pid, time_left });
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

bool q_zero_and_one(int qn, int time_quantom) {

	Process now_p = q[qn].front();
	int pid = now_p.PID;
	cout << "q"<< qn <<" now pid: " << pid << endl;
	int now_burst_num = now_burst[pid];


	//CPU �� bt 1 ����
	if ((now_burst_num+2) % 2 == 0) { // ¦��: CPU burst time
		//���� ���μ����� ���� �����ִ� burst time ���ҽ�Ŵ 
		q[qn].front().BTs[now_burst_num]--;
		processes[pid].BTs[now_burst_num]--;
	}
	else {
		//�� ó�� IO ������ Ÿ�̹��̶�� sleep ��Ű�� �ѱ�
		push_IO(qn, pid, now_p.BTs[now_burst[pid]]);
		q[qn].pop_front();
		return false; //�ѱ�� ��������.
	}

	//time_quantom �� ������ ���μ����� ���� ť�� �ѱ��
	if (time_quantom == 0) {
		cout << "time quantom out! " << endl;
		//time quantom�� �� ����, �ð��� �� �� ������ ��ȣ + ����� ��
		if (processes[pid].BTs[now_burst_num] == 0) {
			now_burst[pid]++;
			//�� �°� ���μ��� ����Ǵ� ���̽�
			if (now_burst[pid] == now_p.BTs.size()) {
				q[qn].pop_front();
				completed++;
			}
		}
		if (qn == 0) q[qn + 1].push_back(processes[pid]);
		else if (qn == 1) q2.push(processes[pid]);
		print_q2();
		q[qn].pop_front();
		return false; //q1�۾� ��
	}

	//CPU burst �� ���� IO burst�� �Ѿ��
	else if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;
			
		//�ƿ� ���μ����� ���̸� �����ϱ�
		if (now_burst[pid] == now_p.BTs.size()) {
			cout << "process " << pid << " is ended." << endl;
			q[qn].pop_front();
			completed++;
		}
		//���� �ƴϸ� IO�� �Ѿ
		else {
			cout << "process " << pid << " switched to IO mode " << endl;
			push_IO(qn, pid, processes[pid].BTs[now_burst[pid]] );
			q[qn].pop_front();
		}
		return false; //q1�۾� ��
	}
	return true;
}
void print_ios() {
	for (auto io : sorted_IO) {
		cout << "io.pid: " << io[1] << ", left time: " << processes[io[1]].BTs[now_burst[io[1]]] <<" ";
	}
}
//1�ʿ� �ѹ��� ������ �Լ�.
void IO() {
	print_ios();
	for (auto io : sorted_IO) {
		cout << "io pid: " << io[1] << endl;
		io[2]--;
		processes[io[1]].BTs[now_burst[io[1]]]--;
	}
	for (int i = 0; i< sorted_IO.size() ; i ++) {
		//0�̶� wakeup �ؾ��ϸ�? will wake up �� �־��� // 1 upper queue�� �־���
		if (processes[sorted_IO[i][1]].BTs[now_burst[sorted_IO[i][1]]] == 0) {
			if (sorted_IO[i][0] > 0) {
				sorted_IO[i][0]--;
			}
			will_wake_up.push(sorted_IO[i]);
			cout << "pushed " << sorted_IO[i][1] << " to will wake up" << endl;
			sorted_IO.erase(sorted_IO.begin()+i);
		}
	}

}

void wake_up() {
	while (!will_wake_up.empty()) {
		vector<int> t = will_wake_up.front();
		will_wake_up.pop();

		cout << t[1] << " woke up!" << endl;
		//burst ��ȣ �÷���
		now_burst[t[1]]++;
		if (t[0] != 2) q[t[0]].push_back(processes[t[1]]);
		else { // preemtion�� �Ͼ�� ��
			int q2_pn = q2.top().PID;
			if (processes[t[1]].BTs[now_burst[t[1]]] < q2.top().BTs[now_burst[q2_pn]]) { //���� ���� ���� �ְ� ���� �ϰ��ִ� q2 ���μ������� ª�� �������� preemtion�� �Ͼ.
				cout << "preemtion is occurred in q2 " << endl;
				q[3].push_back(q2.top());
				q2.pop();
			}
			else { //preemtion �Ͼ�� �ʴ� ���
				q2.push(processes[t[1]]);
			}
		}
	}
}

bool q_two() {

	Process now_p = q2.top();
	int pid = now_p.PID;
	cout << "q2 now pid: " << pid << endl;
	int now_burst_num = now_burst[pid];


	//CPU �� bt 1 ����
	if ((now_burst_num + 2) % 2 != 0) cout << "q2 burst num is odd. something wrong" << endl;
	//���� ���μ����� ���� �����ִ� burst time ���ҽ�Ŵ 
	now_p.BTs[now_burst_num] -=1;
	processes[pid].BTs[now_burst_num] -= 1;

	//CPU burst �� ���� IO burst�� �Ѿ��
	if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;

		//�ƿ� ���μ����� ���̸� �����ϱ�
		if (now_burst[pid] == processes[pid].BTs.size()) {
			cout << "in q2: process " << pid << " is ended." << endl;
			q2.pop();
			completed++;
		}
		//���� �ƴϸ� IO�� �Ѿ
		else {
			cout << "in q2: process " << pid << " switched to IO mode " << endl;
			push_IO(2, pid, processes[pid].BTs[now_burst[pid]]);
			q2.pop();
		}
		return false; // q2 �۾� ������ return false
	}
	return true;
}

bool q_three() {
	Process now_p = q[3].front();
	int pid = now_p.PID;
	cout << "q3 now pid: " << pid << endl;
	int now_burst_num = now_burst[pid];

	//CPU �� bt 1 ����
	if ((now_burst_num + 2) % 2 != 0) cout << "q3 burst num is odd. something wrong" << endl;
	//���� ���μ����� ���� �����ִ� burst time ���ҽ�Ŵ 
	q[3].front().BTs[now_burst_num] -= 1;
	processes[pid].BTs[now_burst_num] -= 1;
	if (processes[pid].BTs[now_burst_num] == 0) {
		now_burst[pid]++;

		//�ƿ� ���μ����� ���̸� �����ϱ�
		if (now_burst[pid] == processes[pid].BTs.size()) {
			cout << "process " << pid << " is ended." << endl;
			q[3].pop_front();
			completed++;
		}
		//���� �ƴϸ� IO�� �Ѿ
		else {
			cout << "process " << pid << " switched to IO mode " << endl;
			push_IO(0, pid, now_p.BTs[now_burst[pid]]);
			q[3].pop_front();
		}
		return false; //q3 �۾� ��
	}
	return true;

}
int main() {

	FILE* fp = NULL;
	fp = fopen("input.txt", "r");
	while (!feof(fp)) {
		fscanf(fp, "%d\n", &pn);
		is_sleeping.resize(pn, 0);
		now_burst.resize(pn, 0);
		for (int i = 0; i < pn; i++) {
			Process pp;
			pp.PID = i;
			fscanf(fp, "%d %d %d ", &pp.queue_num, &pp.arrival_time, &pp.cycles_num);
			for (int j = 0; j < pp.cycles_num * 2 - 1; j++) {
				int tmp=0;
				fscanf(fp, "%d ", &tmp);
				pp.BTs.push_back(tmp);
			}
			if (pp.queue_num != 2)q[pp.queue_num].push_back(pp);
			else q2.push(pp);
			processes.push_back(pp);
		}
	}
	
	int time = 0;
	while (completed < processes.size()) {
		
		if (!q[0].empty()) {
			
			int tmp = 2;
			while (tmp--) {
				cout << "time: " << time++ << endl;
				print_processes();
				//�����
				wake_up();
				//IO 1����& 0�Ǹ�ѱ�
				IO();
				if (!q_zero_and_one(0, tmp)) break;
			}
		}
		else if (!q[1].empty()) {
			
			int tmp = 6;
			while (tmp--) {
				cout << "time: " << time++ << endl;
				print_processes();
				//�����
				wake_up();
				//IO 1����& 0�Ǹ�ѱ�
				IO();
				if (!q_zero_and_one(1, tmp)) break;
			}
		}
		else if (!q2.empty()) {
			while (1) {
				cout << "time: " << time++ << endl;
				print_processes();
				//�����
				wake_up();
				//IO 1����& 0�Ǹ�ѱ�
				IO();
				if (!q_two()) break;
			};
		}
		else if (!q[3].empty()) {
			while (1) {
				cout << "time: " << time++ << endl;
				print_processes();
				//�����
				wake_up();
				//IO 1����& 0�Ǹ�ѱ�
				IO();
				if (!q_three()) break;
			};
		}
		else {
			cout << "time: " << time++ << endl;
			cout << " only IO working! " << endl;
			wake_up();
			print_processes();
			IO();
		}
	}
	return 0;
}