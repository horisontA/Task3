#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <stdio.h>
#include "bson/bson.h"
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

void SaveBson(string filename, vector<Fileinfo> vec_finfo){
	mongo::BSONArrayBuilder array;
	for (Fileinfo it : vec_finfo){
		bson::bob element;
		element.append("path", it.path);
		element.append("size", it.size);
		element.append("hash", it.hash);
 
		//mongo::BSONArrayBuilder array1;
		array.append(element.obj());
	}
 
	bson::bo serializedArray = array.arr();
 
	//Add array into more general object
	bson::bob container;
	container.appendArray("elements", serializedArray);
	bson::bo p = container.obj();
	std::ofstream fbson("./fileout.bson", std::ios::binary | std::ios::out | std::ios::trunc);
	fbson.clear();
	fbson.write(p.objdata(), p.objsize());
	fbson.close();
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

void ReadBson(vector<Fileinfo> & vec_finfo){
	Fileinfo it;
	string path_copy;
	string size_copy;
	string hash_copy;
	std::ofstream fbson("./fileout.bson", std::ios::binary | std::ios::in);
	stringstream buffer;
	buffer << fbson.rdbuf();
	fbson.close();
	auto data = buffer.str();
 
	bson::bo b(data.c_str());
	auto name = "elements";
	bson::be elements = b.getFieldDottedOrArray(name);
 
	for (auto & element : elements.Array()) {
		bson::bo data = element.Obj();
		if (data.hasElement("path") && data.hasElement("size") && data.hasElement("hash")) {
			path_copy = data["path"];
			path_copy.erase(0, 7);
			path_copy.erase(path_copy.length() - 1, 1);
			it.path = path_copy;
			size_copy = data["size"];
			size_copy.erase(0, 5);
			it.size = stoi(size_copy);
			hash_copy = data["hash"];
			hash_copy.erase(0, 7);
			hash_copy.erase(hash_copy.length() - 1, 1);
			it.hash = hash_copy;
			vec_finfo.push_back(it);
		}
	}
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
		
		cout << "What do you want: save and check on CSV or BSON?" << endl;
		cout << "If you want to work with CSV: enter CSV. Else: enter BSON." << endl;
		string status;
		getline(cin, status);
		if (status == "CSV"){
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
		if (status == "BSON"){
			if (checkstatus == "1") {
				SaveBson("out.bson", vec_finfo);
				print(vec_finfo);
			}
			if (checkstatus == "2") {
				ReadBson(vec_finfo_old);
				vec_finfo_new = compare_lists(vec_finfo, vec_finfo_old);
				print(vec_finfo_new);
			}
		}
	}
		cin.get();
		return 0;
	}
