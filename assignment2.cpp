#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <ctgmath>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "fn_stem.h"

#define NO_OF_DOCS 100
#define RELEVANT_WD_FACTOR 1
#define K_VALUE 4
#define LEAST_MEMBERS_LIMIT 5
#define PROGRAM_INP_FILENAME "assgn2.inp"
#define INPUT_FOLDER_LOCATION "20ngb_dataset\\foresale\\foresale1\\"
#define INPUT_FOLDER_NAME "foresale1"
#define INPUT_FILENAME "ngb"
#define INPUT_FILE_NO 101
#define INPUT_FILE_EXTENSION ""
#define OUTPUT_FOLDER_NAME "OUT_Assgn2"
#define OUTPUT_FILENAME "_doc_out"
#define FILE_STOP_WORDS "stop_words"

using namespace std;

extern int stem(char *, int, int);

// For debugging only
ofstream temp_fout("prog_testing.txt");
// --------------------------------------

BOOL WINAPI CreateDirectory(
	_In_     LPCTSTR               lpPathName,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);


class TermPosting{
public:
	int doc_no;
	int term_freq;
	double tf_idf;
	TermPosting(int dn, int tf, double ti_df) : doc_no(dn), term_freq(tf), tf_idf(ti_df) {}
};

bool is_alphabet(char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return true;
	else
		return false;
}

void tokenize_and_normalize(vector<string>& words)
{

	for (int wd_pos = 0; wd_pos < words.size(); ++wd_pos){

		//temp_fout << "<" << words[wd_pos] << ", " << words[wd_pos].size() << ">" << endl;

		// Check for ellipses (...)
		if (words[wd_pos].size() >= 3){
			for (int i = 0; i < words[wd_pos].size() - 2; ++i){
				if (words[wd_pos][i] == '.' && words[wd_pos][i + 1] == '.' && words[wd_pos][i + 2] == '.'){
					if (i + 3 < words[wd_pos].size()){
						string temp_str = words[wd_pos];
						temp_str.erase(temp_str.begin(), temp_str.begin() + i + 3);
						words.push_back(temp_str);
					}
					words[wd_pos].erase(words[wd_pos].begin() + i, words[wd_pos].end());
					break;
				}
			}
		}

		// Check for - {WORD}[ . , : ; ? ! " ) } \] ]
		if (words[wd_pos].size()){
			switch (words[wd_pos][words[wd_pos].size() - 1]){
			case('.') : case(',') : case(':') : case(';') : case('?') : case('\'') :
			case('!') : case('"') : case(')') : case('}') : case(']') :
			{
				words[wd_pos].erase(words[wd_pos].end() - 1);
				--wd_pos;
				continue;
			}
			default:
			{
				//cout << words[wd_pos] << ": no punctuations in the end.\n";
				break;
			}
			}
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

		// Check for - [ ( { \[ " ]{WORD}
		if (words[wd_pos].size()){
			switch (words[wd_pos][0]){
			case('(') : case('{') : case('[') : case('"') : case('\'') :
				{
					words[wd_pos].erase(words[wd_pos].begin());
					--wd_pos;
					continue;
				}
			default:
				{
					//cout << words[wd_pos] << ": no punctuations in the beginning.\n";
					break;
				}
			}
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

		// Check for - {WORD}"'s"
		if (words[wd_pos].size() >= 2){
			if (words[wd_pos][words[wd_pos].size() - 2] == '\'' && words[wd_pos][words[wd_pos].size() - 1] == 's'){
				words[wd_pos].erase(words[wd_pos].end() - 2, words[wd_pos].end());
				--wd_pos;
				continue;
			}
			//else
			//cout << words[wd_pos] << ": no \"'s\" in the end.\n";
		}

		// Remove abbr.-periods
		if (words[wd_pos].size()){
			words[wd_pos].erase(remove(words[wd_pos].begin(), words[wd_pos].end(), '.'), words[wd_pos].end());
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

		// Check if this words is a 'stop word'
		if (words[wd_pos].size()){
			ifstream fin_stop_wds(FILE_STOP_WORDS);
			string temp_str;
			bool is_stopwd = false;

			// Convert to lowercase
			transform(words[wd_pos].begin(), words[wd_pos].end(), words[wd_pos].begin(), tolower);

			// Check
			while (fin_stop_wds >> temp_str)
				if (words[wd_pos] == temp_str){
					words.erase(words.begin() + wd_pos);
					fin_stop_wds.close();
					--wd_pos;
					is_stopwd = true;
					break;
				}

			if (is_stopwd)
				continue;

			fin_stop_wds.close();
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

		// Check for word-with-hyphens
		if (words[wd_pos].size()){
			for (int i = 0; i < words[wd_pos].size(); ++i){
				if (words[wd_pos][i] == '-'){
					if (i + 1 < words[wd_pos].size()){
						string temp_str = words[wd_pos];
						temp_str.erase(temp_str.begin(), temp_str.begin() + i + 1);
						words.push_back(temp_str);
					}
					words[wd_pos].erase(words[wd_pos].begin() + i, words[wd_pos].end());
				}
			}
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

		// Do stemming
		if (words[wd_pos].size()){
			// Check if it contains digits or special chars
			bool digit_present = false, special_char_present = false;
			for (int i = 0; i < words[wd_pos].size(); ++i){
				if (isdigit(words[wd_pos][i]))
					digit_present = true;
				if (!is_alphabet(words[wd_pos][i]) && !isdigit(words[wd_pos][i]))
					special_char_present = true;
				if (digit_present && special_char_present)
					break;
			}

			if (!digit_present && !special_char_present){
				// Do stemming
				int i = -1, end_pos = -1;

				char *temp_str = (char *)malloc((words[wd_pos].size() + 1) * sizeof(char));

				for (i = 0; i < words[wd_pos].size(); ++i){
					temp_str[i] = words[wd_pos][i];
				}
				temp_str[i] = '\0';

				end_pos = stem(temp_str, 0, i - 1);

				words[wd_pos] = "";
				for (i = 0; i <= end_pos; ++i)
					words[wd_pos].push_back(temp_str[i]);
			}
			// Remove numbers & only-special-char-containing-strings
			else if ((digit_present && !special_char_present) || (!digit_present && special_char_present)){
				words.erase(words.begin() + wd_pos);
				--wd_pos;
				continue;
			}
			else
				continue;
		}
		else{
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}

	 // Remove single char strings
		if (words[wd_pos].size() == 1){
			words.erase(words.begin() + wd_pos);
			--wd_pos;
			continue;
		}
	}

	return;
}

void manage_uqwd_list(vector<string>& unique_wds, string word_str)
{
	//temp_fout << "#" << word_str << ", " << unique_wds.size() << "#\n";

	// Check if the word_str is present in the list
	bool is_present = false;
	int lo = 0,
		hi = unique_wds.size() - 1,
		mid = 0,
		position = 0;					// Set to 0 for position = 0, to add 1st element to the list

	while (lo <= hi){
		mid = (lo + hi) / 2;

		if (word_str == unique_wds[mid]){
			is_present = true;
			break;
		}
		else if (lo == hi){
			break;
		}
		else if (word_str < unique_wds[mid]){
			hi = mid - 1;
		}
		else if (word_str > unique_wds[mid]){
			lo = mid + 1;
		}
	}

	// If not present, add it to the list
	if (!is_present){

		// Find position where to add this new element, to keep a sorted list of unique words
		if (unique_wds.size()){
			if (word_str < unique_wds[mid]){
				if (mid != 0)
					position = mid;
				else
					position = 0;
			}
			else if (word_str > unique_wds[mid])
				position = mid + 1;
		}

		// Now, add the word
		unique_wds.push_back("");
		for (int i = unique_wds.size() - 2; i >= position; --i)
			unique_wds[i + 1] = unique_wds[i];
		unique_wds[position] = word_str;
	}

	return;
}

// v1 or v2 == 0 => Null vector, non-zero v1 or v2 => a document vector, e.g. v1 == 2 => 2nd document.
double calc_distance(vector<vector<double>>& mtrx, int n_members, int pos1, int pos2)
{
 // Use Euclidean distance instead
	double dist = -1;

	dist = 0;
	if (pos1 != pos2){
		if (pos1 && pos2){
			for (int i = 0; i < n_members; ++i)
				dist += pow((mtrx[pos1 - 1][i] - mtrx[pos2 - 1][i]), 2);
			//dist = sqrt(dist);
		}
		else if (pos1 && !pos2){
			for (int i = 0; i < n_members; ++i)
				dist += pow(mtrx[pos1 - 1][i], 2);
			//dist = sqrt(dist);
		}
		else if (!pos1 && pos2){
			for (int i = 0; i < n_members; ++i)
				dist += pow(mtrx[pos2 - 1][i], 2);
			//dist = sqrt(dist);
		}
	}

	return dist;
}

double calc_distance(vector<vector<double>>& mtrx1, vector<vector<double>>& mtrx2, int n_members, int pos1, int pos2)
{
	// Use Euclidean distance instead
	double dist = -1;

	dist = 0;
	if (pos1 && pos2){
		for (int i = 0; i < n_members; ++i)
			dist += pow((mtrx1[pos1 - 1][i] - mtrx2[pos2 - 1][i]), 2);
		//dist = sqrt(dist);
	}
	else if (pos1 && !pos2){
		for (int i = 0; i < n_members; ++i)
			dist += pow(mtrx1[pos1 - 1][i], 2);
		//dist = sqrt(dist);
	}
	else if (!pos1 && pos2){
		for (int i = 0; i < n_members; ++i)
			dist += pow(mtrx2[pos2 - 1][i], 2);
		//dist = sqrt(dist);
	}

	return dist;
}

int main()
{
	string input_str, temp_str;
	vector<string> words, unique_wds;

 // I/O streams
	ifstream fin, prog_input;
	ofstream fout;
	stringstream ss;

 // Creating tokens, for each document...
	int n_folders = -1, n_docs = -1, doc_name_no = -1, t_n_docs = -1;
	string inp_folder_location, inp_doc_initials, inp_doc_extension, outp_folder_name;

	prog_input.open(PROGRAM_INP_FILENAME);
	prog_input >> n_folders;

	// Create Output Folder
	ss.clear();
	ss << OUTPUT_FOLDER_NAME;
	ss >> outp_folder_name;
	temp_fout << "Outp_folder: " << outp_folder_name << endl;
	if (CreateDirectory(outp_folder_name.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
		temp_fout << "Folder created: " << outp_folder_name << endl;
	else{
		temp_fout << "Unable to create folder: " << outp_folder_name << endl;
		return 0;
	}

	t_n_docs = 0;
	for (int j = 0; j < n_folders; ++j){
		prog_input >> inp_folder_location >> n_docs >> inp_doc_initials >> doc_name_no >> inp_doc_extension;
		cout << "Folder No. = " << j << endl;
		for (int i = 1; i <= n_docs; ++i){
			++t_n_docs;
			cout << "  Document No. = " << i << endl;

			// Set input & output stream
			// For input file
			ss.clear();
			ss << inp_folder_location << inp_doc_initials << doc_name_no << inp_doc_extension;
			ss >> temp_str;
			//temp_fout << "Inp_file: " << temp_str << endl;
			fin.open(temp_str.c_str());

			// For output file
			ss.clear();
			ss << outp_folder_name << "\\" << t_n_docs << OUTPUT_FILENAME << ".txt";
			ss >> temp_str;
			//temp_fout << "Outp_file: " << temp_str << endl;
			fout.open(temp_str.c_str());

			// Read a string, create token(s) out of it
			while (fin >> input_str){

				// Empty vector 'words' & then add 'input_str' to 'words'
				words.clear();
				words.push_back(input_str);

				// Create token(s) - also removes stop words & store it in a file
				tokenize_and_normalize(words);
				for (int i = 0; i < words.size(); ++i){
					// Manage unique words' list
					manage_uqwd_list(unique_wds, words[i]);

					fout << words[i] << "\n";
				}
			}

			++doc_name_no;

			fin.close();
			fout.close();
		}
	}

 // Show me those unique words
	//temp_fout << "\nUnique Words' List - " << unique_wds.size() << "\n";
	//for (int i = 0; i < unique_wds.size(); ++i){
	//	temp_fout << unique_wds[i] << endl;
	//}
	//temp_fout << endl;

 // Determine & manage term frequency, using inverted index
	vector<vector<TermPosting>> inverted_indx(unique_wds.size());
	temp_fout << inverted_indx.size() << endl;

	// From each document...
	for (int i = 1; i <= t_n_docs; ++i){
		// Set input stream, use the output file containing tokens
		ss.clear();
		ss << outp_folder_name << "\\" << i << OUTPUT_FILENAME << ".txt";
		ss >> temp_str;
		fin.open(temp_str.c_str());

		//temp_fout << "fin.open: " << temp_str << endl;
		// Read words, manage term-frequency
		while (fin >> input_str){

			// Find its position in the list of unique words
			int lo = 0,
				hi = unique_wds.size() - 1,
				mid = -1,
				position_wd = -1;
			while (lo <= hi){
				mid = (lo + hi) / 2;
				if (input_str == unique_wds[mid])
					break;
				else if (input_str < unique_wds[mid])
					hi = mid - 1;
				else if (input_str > unique_wds[mid])
					lo = mid + 1;
			}
			position_wd = mid;

			//temp_fout << i << ": wd: " << unique_wds[position_wd] << ", pos_wd: " << position_wd << endl;

			// Check if the list of 'input_str' contains doc 'i'
			int position_doc = -1;
			lo = 0;
			hi = inverted_indx[position_wd].size() - 1;
			mid = 0;	// Set to 0 for 'position_doc = mid' when 'position_doc < 0'
			while (lo <= hi){
				mid = (lo + hi) / 2;
				//temp_fout << "lo: " << lo << ", hi: " << hi << ", mid: " << mid << endl;
				if (i == inverted_indx[position_wd][mid].doc_no){
					position_doc = mid;
					break;
				}
				else if (lo == hi)
					break;
				else if (i < inverted_indx[position_wd][mid].doc_no)
					hi = mid - 1;
				else if (i > inverted_indx[position_wd][mid].doc_no)
					lo = mid + 1;
			}

			//temp_fout << "pos_doc: " << position_doc << ", mid: " << mid << endl;

			// If not present in the list, add it
			if (position_doc < 0){
				// Determine its position to keep the list sorted
				if (inverted_indx[position_wd].size()){
					if (i < inverted_indx[position_wd][mid].doc_no)
						position_doc = mid;
					else if (i > inverted_indx[position_wd][mid].doc_no)
						position_doc = mid + 1;
				}
				else{
					position_doc = 0;
				}

				//temp_fout << "term not present: adding to position = " << position_doc << endl;

				// Add 'i' document to the list
				inverted_indx[position_wd].push_back(TermPosting(-1, -1, -1));
				for (int j = inverted_indx[position_wd].size() - 2; j >= position_doc; --j)
					inverted_indx[position_wd][j + 1] = inverted_indx[position_wd][j];
				inverted_indx[position_wd][position_doc].doc_no = i;
				inverted_indx[position_wd][position_doc].term_freq = 1;
				inverted_indx[position_wd][position_doc].tf_idf = -1;
			}
			// If present, increment frequency
			else if (position_doc >= 0 && position_doc <= inverted_indx[position_wd].size() - 1){
				++inverted_indx[position_wd][position_doc].term_freq;
				//temp_fout << "term present: incremented at position = " << position_doc << ", tf: " << inverted_indx[position_wd][position_doc].term_freq << endl;
			}
		}

		fin.close();
	}

	// Term frequency of every unique term for every document
	for (int i = 1; i <= t_n_docs; ++i){
		int count_wds = 0;
		for (int j = 0; j < inverted_indx.size(); ++j){
			int position = -1,
				lo = 0,
				hi = inverted_indx[j].size() - 1,
				mid = -1;
			while (lo <= hi){
				mid = (lo + hi) / 2;
				if (i == inverted_indx[j][mid].doc_no){
					//temp_fout << "@" << i << ":" << unique_wds[j] << ", tf:" << inverted_indx[j][mid].term_freq << "@\n";
					count_wds += inverted_indx[j][mid].term_freq;
					break;
				}
				else if (i < inverted_indx[j][mid].doc_no)
					hi = mid - 1;
				else if (i > inverted_indx[j][mid].doc_no)
					lo = mid + 1;
			}
		}
		//temp_fout << "\nWord Count (" << i << "): " << count_wds << endl;
	}

	// Print term frequency matrix
	for (int i = 0; i < inverted_indx.size(); ++i){		// For every term
		temp_fout << setw(15) << unique_wds[i] << "(" << setw(3) << i << "): ";
		for (int j = 0; j < inverted_indx[i].size(); ++j){
			if (j == 0 && inverted_indx[i][j].doc_no != 1)
				for (int k = 1; k < inverted_indx[i][j].doc_no; ++k)
					temp_fout << setw(3) << "0";
			if (j != 0 && inverted_indx[i][j].doc_no != inverted_indx[i][j - 1].doc_no + 1)
				for (int k = 1; k <= inverted_indx[i][j].doc_no - inverted_indx[i][j - 1].doc_no - 1; ++k)
					temp_fout << setw(3) << "0";
			temp_fout << setw(3) << inverted_indx[i][j].term_freq;
			if (j == inverted_indx[i].size() - 1 && inverted_indx[i][j].doc_no != t_n_docs)
				for (int k = inverted_indx[i][j].doc_no + 1; k <= t_n_docs; ++k)
					temp_fout << setw(3) << "0";
		}
		temp_fout << endl;
	}
	temp_fout << endl;

 // Determine relevant terms
	// Calculate tf-idf for every term for every document
	vector<double> tf_idf_wd_avg(inverted_indx.size(), 0);	// vector containing avg tf-idf values for each unique word
	double tf_idf_overall_avg = 0;							// overall average of tf-idf
	for (int i = 0; i < inverted_indx.size(); ++i){
		for (int j = 0; j < inverted_indx[i].size(); ++j){
			inverted_indx[i][j].tf_idf = inverted_indx[i][j].term_freq * (log((double)t_n_docs / inverted_indx[i].size()));
			tf_idf_wd_avg[i] += inverted_indx[i][j].tf_idf;
		}
		//temp_fout << "wd: " << unique_wds[i] << ", +tf-idf: " << tf_idf_wd_avg[i];
		tf_idf_wd_avg[i] /= t_n_docs;
		//temp_fout << ", tf-idf-avg: " << tf_idf_wd_avg[i];
		tf_idf_overall_avg += tf_idf_wd_avg[i];
		//temp_fout << ", +tf-idf-ovrl: " << tf_idf_overall_avg << endl;
	}
	tf_idf_overall_avg /= inverted_indx.size();
	//temp_fout << "tf-idf-ovrl-avg: " << tf_idf_overall_avg << endl;

	// Print tf-idf frequency matrix
	for (int i = 0; i < inverted_indx.size(); ++i){		// For every term
		temp_fout << setw(15) << unique_wds[i] << "(" << setw(3) << i << "): ";
		for (int j = 0; j < inverted_indx[i].size(); ++j){
			if (j == 0 && inverted_indx[i][j].doc_no != 1)
				for (int k = 1; k < inverted_indx[i][j].doc_no; ++k)
					temp_fout << setw(10) << "0";
			if (j != 0 && inverted_indx[i][j].doc_no != inverted_indx[i][j - 1].doc_no + 1)
				for (int k = 1; k <= inverted_indx[i][j].doc_no - inverted_indx[i][j - 1].doc_no - 1; ++k)
					temp_fout << setw(10) << "0";
			temp_fout << setw(10) << inverted_indx[i][j].tf_idf;
			if (j == inverted_indx[i].size() - 1 && inverted_indx[i][j].doc_no != t_n_docs)
				for (int k = inverted_indx[i][j].doc_no + 1; k <= t_n_docs; ++k)
					temp_fout << setw(10) << "0";
		}
		temp_fout << endl;
	}
	temp_fout << endl;

	// Find the relevant terms & store them - Stores position of unique words which are relevant in 'relv_wds'
	vector<int> relv_wds;
	for (int i = 0; i < tf_idf_wd_avg.size(); ++i)
		if (tf_idf_wd_avg[i] >= tf_idf_overall_avg * RELEVANT_WD_FACTOR)
			relv_wds.push_back(i);	// i == position of word in the list of unique words
	temp_fout << "no. of relevant words: " << relv_wds.size() << endl;
	
	//cout << "Done1\n";

	// Create a document-relevant-term-tf-idf matrix
	vector<vector<double>> incident_mtrx(t_n_docs, vector<double>(relv_wds.size()));
	//cout << "Done2\n";
	for (int i = 0; i < relv_wds.size(); ++i){
		//cout << i << " " << inverted_indx[relv_wds[i]].size() << " " << relv_wds[i] << ": Done3\n";
		for (int j = 0; j < inverted_indx[relv_wds[i]].size(); ++j){
			//cout << i << " " << j << " " << inverted_indx[relv_wds[i]][j].doc_no << ": Done4\n";
			incident_mtrx[inverted_indx[relv_wds[i]][j].doc_no - 1][i] = inverted_indx[relv_wds[i]][j].tf_idf;
			//cout << i << " " << j << " " << inverted_indx[relv_wds[i]][j].term_freq << "Done5\n";
		}
	}
	//cout << "Done6\n";

	// Print the incident matrix
	temp_fout << endl << setw(15) << "  ";
	for (int i = 0; i < incident_mtrx.size(); ++i)
		temp_fout << setw(10) << i << " ";
	temp_fout << endl;
	for (int i = 0; i < relv_wds.size(); ++i){
		temp_fout << setw(15) << unique_wds[relv_wds[i]] << ": ";
		for (int j = 0; j < incident_mtrx.size(); ++j)
			temp_fout << setw(10) << incident_mtrx[j][i] << " ";
		temp_fout << endl;
	}

 // K-Means Clustering

/*	// Find nearest & farthest point
	double closest_pt_dist = numeric_limits<double>::infinity(),
		  farthest_pt_dist = numeric_limits<double>::lowest(),
		  temp_dist = numeric_limits<double>::lowest();
	int closest_pt = -1, farthest_pt = -1;
	temp_fout << endl;
	for (int i = 1; i <= NO_OF_DOCS; ++i){
		temp_dist = calc_distance(incident_mtrx, relv_wds.size(), 0, i);
		temp_fout << "dist(" << i << "): " << temp_dist << endl;
		if (temp_dist < closest_pt_dist){
			closest_pt_dist = temp_dist;
			closest_pt = i;
		}
		if (temp_dist > farthest_pt_dist){
			farthest_pt_dist = temp_dist;
			farthest_pt = i;
		}
	}

	temp_fout << "cl_pt: " << closest_pt << ", " << closest_pt_dist << ", ft_pt: " << farthest_pt << ", " << farthest_pt_dist << endl;

	// Print closest & farthest pt vectors
	temp_fout << "\nCl Pt Vtr: ";
	for (int i = 0; i < relv_wds.size(); ++i)
		temp_fout << incident_mtrx[closest_pt - 1][i] << " ";
	temp_fout << "\nFr Pt Vtr: ";
	for (int i = 0; i < relv_wds.size(); ++i)
		temp_fout << incident_mtrx[farthest_pt - 1][i] << " ";
	temp_fout << endl;
*/
	// Create a set of centroids
	//double *v_centroids = (double*)malloc(relv_wds.size() * K_VALUE);
	vector<vector<double>> v_centroids(K_VALUE, vector<double>(relv_wds.size()));

/*	// Set closest & farthest centroids
	for (int i = 0; i < relv_wds.size(); ++i){		// closest pt vector in 1st row, 0 index
		//*(v_centroids + (0 * relv_wds.size()) + i) = incident_mtrx[i][closest_pt];
		v_centroids[0][i] = incident_mtrx[closest_pt - 1][i];
	}
	for (int i = 0; i < relv_wds.size(); ++i){		// farthest pt vector in last row, (K_VALUE - 1) index
		//*(v_centroids + ((K_VALUE - 1) * relv_wds.size()) + i) = incident_mtrx[i][farthest_pt];
		v_centroids[K_VALUE - 1][i] = incident_mtrx[farthest_pt - 1][i];
	}
	for (int j = 1; j < K_VALUE - 1; ++j){			// create rest of the centroids b/w these 2 pts
		for (int i = 0; i < relv_wds.size(); ++i){
			//cout << "TO-DO: " << i << ", " << j << endl;
			//*(v_centroids + (j * relv_wds.size()) + i) = (*(v_centroids + ((K_VALUE - 1) * relv_wds.size()) + i) - *(v_centroids + i)) * j / (K_VALUE - 1);
			v_centroids[j][i] = (v_centroids[K_VALUE - 1][i] - v_centroids[0][i]) * j / (K_VALUE - 1);
			//cout << "DONE: " << i << ", " << j << endl;
		}
	}
*/

	// Select centroids randomly
	int temp_centroid = -1;
	vector<int> rand_centroid;
	srand(time(NULL));
	while (rand_centroid.size() != K_VALUE){
		//temp_fout << rand_centroid.size() << endl;
		temp_centroid = rand() % t_n_docs;

		int lo = 0, hi = rand_centroid.size() - 1, mid = 0, pos = -1;
		while (lo <= hi){
			mid = (lo + hi) / 2;
			if (temp_centroid == rand_centroid[mid]){
				pos = mid;
				break;
			}
			else if (lo == hi)
				break;
			else if (temp_centroid < rand_centroid[mid])
				hi = mid - 1;
			else if (temp_centroid > rand_centroid[mid])
				lo = mid + 1;
		}

		if (pos < 0){
			if (rand_centroid.size()){
				if (temp_centroid < rand_centroid[mid])
					pos = mid;
				else if (temp_centroid > rand_centroid[mid])
					pos = mid + 1;
			}
			else
				pos = 0;

			rand_centroid.push_back(-1);
			for (int j = rand_centroid.size() - 2; j >= pos; --j)
				rand_centroid[j + 1] = rand_centroid[j];
			rand_centroid[pos] = temp_centroid;
		}
	}

	//temp_fout << "Done.\n";

	// Print the centroids selected
	temp_fout << endl;
	for (int i = 0; i < rand_centroid.size(); ++i)
		temp_fout << setw(3) << rand_centroid[i] << " ";
	temp_fout << endl;

/*
	// Add centroids manually
	vector<int> man_centroid(K_VALUE);
	man_centroid[0] = 10;
	man_centroid[1] = 30;
	man_centroid[2] = 60;
	man_centroid[3] = 90;
*/

	// Put centroids' vectors in 'v_centroids'
	for (int i = 0; i < K_VALUE; ++i)
		for (int j = 0; j < relv_wds.size(); ++j)
			v_centroids[i][j] = incident_mtrx[rand_centroid[i]][j];

	// Print vector of centroids
	//temp_fout << endl << "Centroid-Matrix: " << relv_wds.size() << "\n";
	//for (int i = 0; i < relv_wds.size(); ++i){
	//	temp_fout << setw(2) << i << ": ";
	//	for (int j = 0; j < K_VALUE; ++j){
	//		temp_fout << setw(20) << *(v_centroids + (j * relv_wds.size()) + i) << " ";
	//		temp_fout << setw(20) << v_centroids[j][i] << " ";
	//	}
	//	temp_fout << endl;
	//}

 // Apply k-means clustering
	bool any_centroid_changed = false;
	int count_itrtn = 0;
	//double *k_clusters = (double*)malloc(K_VALUE * NO_OF_DOCS);
	vector<vector<double>> k_clusters(t_n_docs, vector<double>(K_VALUE));
	do{
		++count_itrtn;
	 // Create cluster
		for (int i = 0; i < t_n_docs; ++i){	// For each document
			int clst_centroid = -1;
			double clst_centroid_dist = numeric_limits<double>::infinity(), temp_dist = -1;
			for (int j = 0; j < K_VALUE; ++j){
				temp_dist = calc_distance(incident_mtrx, v_centroids, relv_wds.size(), i + 1, j + 1);
				temp_fout << "doc(" << i << "," << j << "): " << temp_dist << endl;
				if (temp_dist != 0){
					if (temp_dist < clst_centroid_dist){
						clst_centroid_dist = temp_dist;
						clst_centroid = j;
					}
				}
				else{
					clst_centroid_dist = -1;
					clst_centroid = j;
					break;
				}
			}
			temp_fout << "doc(" << i << ") => " << "clst_dist: " << clst_centroid_dist << ", clst_centroid: " << clst_centroid << endl;
			for (int j = 0; j < K_VALUE; ++j){
				if (j == clst_centroid)
					k_clusters[i][j] = clst_centroid_dist;
				else
					k_clusters[i][j] = 0;
			}
		}
		
	// Re-calculate centroids
		any_centroid_changed = false;
		vector<double> temp_v_centroid(relv_wds.size(), 0);
		for (int j = 0; j < K_VALUE; ++j){		// For each cluster, add all its component-vectors, dimn-by-dimn
			fill(temp_v_centroid.begin(), temp_v_centroid.end(), 0);
			int count_members = 0;
			for (int k = 0; k < t_n_docs; ++k){
				if (k_clusters[k][j] != 0){
					++count_members;
					for (int l = 0; l < relv_wds.size(); ++l)
						temp_v_centroid[l] += incident_mtrx[k][l];
				}
			}
			if (count_members > LEAST_MEMBERS_LIMIT){
				for (int k = 0; k < relv_wds.size(); ++k)
					temp_v_centroid[k] /= count_members;
				for (int k = 0; k < relv_wds.size(); ++k){
					if (temp_v_centroid[k] != v_centroids[j][k])
						any_centroid_changed = true;
					v_centroids[j][k] = temp_v_centroid[k];
				}
			}
			else{
				any_centroid_changed = true;
				int temp_centroid = rand() % t_n_docs;
				for (int k = 0; k < relv_wds.size(); ++k)
					v_centroids[j][k] = incident_mtrx[temp_centroid][k];
			}
		}

		// Print vector of centroids
		temp_fout << endl << "Centroid-Matrix: " << relv_wds.size() << "\n";
		for (int i = 0; i < relv_wds.size(); ++i){
			temp_fout << setw(2) << i << ": ";
			for (int j = 0; j < K_VALUE; ++j){
				//temp_fout << setw(20) << *(v_centroids + (j * relv_wds.size()) + i) << " ";
				temp_fout << setw(20) << v_centroids[j][i] << " ";
			}
			temp_fout << endl;
		}

		// Print clusters
		temp_fout << endl;
		for (int i = 0; i < t_n_docs; ++i){
			temp_fout << setw(3) << i << ": ";
			for (int j = 0; j < K_VALUE; ++j)
				temp_fout << setw(10) << k_clusters[i][j] << " ";
			temp_fout << endl;
		}

	} while (any_centroid_changed && count_itrtn < 100);

 // For debugging only ------------------------------------------------------------------------------------------------
/*	// Term frequency of every unique term for every document
	for (int i = 1; i <= NO_OF_DOCS; ++i){
		int count_wds = 0;
		for (int j = 0; j < inverted_indx.size(); ++j){
			int position = -1,
				lo = 0,
				hi = inverted_indx[j].size() - 1,
				mid = -1;
			while (lo <= hi){
				mid = (lo + hi) / 2;
				if (i == inverted_indx[j][mid].doc_no){
					//temp_fout << "@" << i << ":" << unique_wds[j] << ", tf:" << inverted_indx[j][mid].term_freq << "@\n";
					count_wds += inverted_indx[j][mid].term_freq;
					break;
				}
				else if (i < inverted_indx[j][mid].doc_no)
					hi = mid - 1;
				else if (i > inverted_indx[j][mid].doc_no)
					lo = mid + 1;
			}
		}
		//temp_fout << "\nWord Count (" << i << "): " << count_wds;
	}
	// TF-IDF Average
	int relv_wd_count = 0;
	double tfidf_ovrall_avg = 0;
	for (int i = 0; i < inverted_indx.size(); ++i){
		double tfidf_wd_avg = 0;
		for (int j = 0; j < inverted_indx[i].size(); ++j){
			tfidf_wd_avg += inverted_indx[i][j].term_freq * (log10((double) NO_OF_DOCS / inverted_indx[i].size()));
		}
		temp_fout << "wd: " << unique_wds[i] << ", +tf-idf: " << tfidf_wd_avg << ", tf-idf-avg: " << tfidf_wd_avg / NO_OF_DOCS << endl;
		tfidf_ovrall_avg += tfidf_wd_avg / NO_OF_DOCS;
		if (tfidf_wd_avg / NO_OF_DOCS >= 0.17)
			++relv_wd_count;
	}
	temp_fout << "tf-idf overall avg: " << tfidf_ovrall_avg / unique_wds.size() << endl;
	temp_fout << "relv_wds: " << relv_wd_count << endl;
	// Average term frequency
	relv_wd_count = 0;
	double avg_of_avg = 0, t_fq = 0, t_dfq = 0;
	for (int i = 0; i < inverted_indx.size(); ++i){
		int tot_fq = 0;
		for (int j = 0; j < inverted_indx[i].size(); ++j)
			tot_fq += inverted_indx[i][j].term_freq;
		//temp_fout << "wd: " << unique_wds[i] << ", tot_fq: " << tot_fq << ", n_docs: " << inverted_indx[i].size() << ", avg_fq: " << (double)tot_fq / NO_OF_DOCS << endl;
		avg_of_avg += (double)tot_fq / NO_OF_DOCS;
		t_fq += tot_fq;
		t_dfq += inverted_indx[i].size();
		if ((double)tot_fq / NO_OF_DOCS >= 0.32)
			++relv_wd_count;
	}
	avg_of_avg /= inverted_indx.size();
	temp_fout << "AoA: " << avg_of_avg << ", TAv: " << t_fq / t_dfq << endl;
	temp_fout << "n_uqwds: " << unique_wds.size() << endl;
	temp_fout << "relv_wds: " << relv_wd_count << endl;
*/
	
//--------------------------------------------------------------------------------------------------------------------

	temp_fout.close();
	cout << "END\n";

	char ch;
	while (cin >> ch)
		if (ch == 'q')
			break;
	return 0;
}