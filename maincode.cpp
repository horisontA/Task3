#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <stdio.h>
#include "keccak.h"
#include <sstream>

using namespace std;
namespace fs = boost::filesystem;

string test;

struct Fileinfo {
	string path;
	string hash;
	int size;
	string flag;
};

vector<Fileinfo> compare_lists(vector<Fileinfo> newfl, vector<Fileinfo> oldfl) {
	for (vector<Fileinfo>::iterator itnew = newfl.begin(); itnew < newfl.end(); itnew++) {
		for (vector<Fileinfo>::iterator itold = oldfl.begin(); itold < oldfl.end(); itold++) {
			if (itnew->hash == itold->hash) {
				itnew->flag = "UNCHANGED";
				oldfl.erase(itold);
				break;
			}
			if (itnew->hash != itold->hash) {
				itnew->flag = "CHANGED";
				oldfl.erase(itold);
				break;
			}
		}
	}
	for (vector<Fileinfo>::iterator itold = oldfl.begin(); itold < oldfl.end(); itold++) {
		itold->flag = "DELETED";
		newfl.push_back(*itold);
	}
	return newfl;
}

void SaveCSV(vector<Fileinfo> vec_finfo){
	ofstream fout("result.csv");
	for (Fileinfo it : vec_finfo){
		fout << it.path << ";" << it.size << ";" << it.hash << ";" << endl;
	}
	fout.close();
	cout << "\n" << "result.csv in project current directory" << endl;
}

void get_dir_list(fs::directory_iterator iterator, std::vector<Fileinfo> * vec_finfo) {
	Fileinfo finfo;
	for (; iterator != fs::directory_iterator(); ++iterator)
	{
		if (fs::is_directory(iterator->status())) {
			fs::directory_iterator sub_dir(iterator->path());
			get_dir_list(sub_dir, vec_finfo);

		}
		else
		{
			finfo.path = iterator->path().string();
			finfo.size = fs::file_size(iterator->path());
			// SHA3
			stringstream result;
			string a;
			ifstream myfile;
			myfile.open(finfo.path, ios::binary);
			result << myfile.rdbuf();
			a = result.str();
			Keccak keccak;
			finfo.hash = keccak(a);
			finfo.flag = "NEW";
			vec_finfo->push_back(finfo);
		}

	}
}

void ReadCSV(vector<Fileinfo> & vec_finfo){
	ifstream file;
	file.open("result.csv", ios::in);
	Fileinfo it;
	string size_copy;
	while (file.good()){
		getline(file, it.path, ';');
		getline(file, size_copy, ';');
		it.size = stoi(size_copy);
		getline(file, it.hash, ';');
		it.flag = "NEW";
		vec_finfo.push_back(it);
	}
	vec_finfo.pop_back();
}

void print(vector<Fileinfo> vec){
	for (Fileinfo it : vec){
		cout << it.path << " : " << it.size << " : " << it.hash << " : " << it.flag << endl;
	}
}

int main(){
	string path, dirpath;
	cout << "What do you want: save(press 1) or check(press 2)?" << endl;
	string checkstatus;

	getline(cin, checkstatus);

	cout << "Enter path ->" << endl;
	vector<Fileinfo> vec_finfo;
	vector<Fileinfo> vec_finfo_old;
	vector<Fileinfo> vec_finfo_new;

	getline(cin, path);


	if (!(fs::exists(path))) {
		cout << "The directory not found. Try again." << endl;
		checkstatus = "null";
	}

	else {
		fs::directory_iterator home_dir(path);
		get_dir_list(home_dir, &vec_finfo);

		if (checkstatus == "1") {
			SaveCSV(vec_finfo);
			print(vec_finfo);
		}
		if (checkstatus == "2") {
			ReadCSV(vec_finfo_old);
			vec_finfo_new = compare_lists(vec_finfo, vec_finfo_old);
			print(vec_finfo_new);
		//	vector<Fileinfo> vec_finfo_old;
			//compare_lists(vec_finfo, vec_finfo_old);
			
		}
	}
		cin.get();
		return 0;
	}