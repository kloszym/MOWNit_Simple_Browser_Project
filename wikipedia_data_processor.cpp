#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <Eigen/Sparse>

#include <curl/curl.h>

#include <nlohmann/json.hpp>

#include <Spectra/GenEigsSolver.h>
#include <Spectra/MatOp/SparseGenMatProd.h>
#include <Spectra/MatOp/SparseSymMatProd.h>
#include <Spectra/SymEigsSolver.h>
#include <Spectra/Util/CompInfo.h>
#include <Spectra/Util/SelectionRule.h>

#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringConverter>
#include <QTextStream>

#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

struct FetchedArticleMeta {
	std::string filename_on_disk_with_ext;
	std::string original_api_title;
};

const std::string ARTICLES_DIR = "wikipedia_articles/";
const int DEFAULT_NUM_ARTICLES_TO_FETCH = 20;
const int ARTICLES_PER_REQUEST = 10;

#ifdef _WIN32
std::wstring utf8_to_wstring_win(const std::string &str) {
	if (str.empty())
		return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int) str.size(), NULL, 0);
	if (size_needed == 0) {

		return std::wstring();
	}
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int) str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}
#endif

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s) {
	size_t newLength = size * nmemb;
	try {
		s->append((char *) contents, newLength);
	} catch (const std::bad_alloc &e) {
		std::cerr << "WriteCallback: bad_alloc caught: " << e.what() << std::endl;
		return 0;
	}
	return newLength;
}

int fetch_and_save_random_wikipedia_articles(int num_articles_to_fetch_this_session,
											 std::vector<FetchedArticleMeta> &session_metadata_output,
											 int &total_articles_ever_fetched_display_counter) {

	if (num_articles_to_fetch_this_session <= 0) {
		return 0;
	}

	CURL *curl_handle;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();

	if (!curl_handle) {
		std::cerr << "Error initializing curl." << std::endl;
		curl_global_cleanup();
		return 0;
	}

	if (!std::filesystem::exists(ARTICLES_DIR)) {
		try {
			if (!std::filesystem::create_directories(ARTICLES_DIR)) {
				std::cerr << "Error: Could not create directory " << ARTICLES_DIR << std::endl;
				curl_easy_cleanup(curl_handle);
				curl_global_cleanup();
				return 0;
			}
		} catch (const std::filesystem::filesystem_error &e) {
			std::cerr << "Filesystem error creating directory " << ARTICLES_DIR << ": " << e.what() << std::endl;
			curl_easy_cleanup(curl_handle);
			curl_global_cleanup();
			return 0;
		}
	}

	std::cout << "Fetching up to " << num_articles_to_fetch_this_session << " random Wikipedia articles..."
			  << std::endl;
	std::string readBuffer;
	int articles_saved_this_session_count = 0;

	while (articles_saved_this_session_count < num_articles_to_fetch_this_session) {
		int articles_to_request_now =
				std::min(ARTICLES_PER_REQUEST, num_articles_to_fetch_this_session - articles_saved_this_session_count);
		if (articles_to_request_now <= 0)
			break;

		readBuffer.clear();
		std::string url = "https://en.wikipedia.org/w/"
						  "api.php?action=query&format=json&generator=random&grnnamespace=0&grnlimit=" +
						  std::to_string(articles_to_request_now) + "&prop=extracts&explaintext=true&exchars=15000";

		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "LSI_CPP_DataProcessor/1.3 (educational_project)");
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60L);
		CURLcode res_code = curl_easy_perform(curl_handle);

		if (res_code != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res_code) << "\nURL: " << url
					  << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(5));
			continue;
		}

		try {
			json j_data = json::parse(readBuffer);
			if (j_data.contains("query") && j_data["query"].contains("pages")) {
				for (auto &[page_id_str, page_obj]: j_data["query"]["pages"].items()) {
					if (articles_saved_this_session_count >= num_articles_to_fetch_this_session)
						break;
					if (page_obj.contains("title") && page_obj["title"].is_string() && page_obj.contains("extract") &&
						page_obj["extract"].is_string()) {

						std::string original_api_title_str = page_obj["title"];
						std::string article_extract_str = page_obj["extract"];

						std::string filename_base = original_api_title_str;
						std::replace(filename_base.begin(), filename_base.end(), ' ', '_');
						std::string invalid_fs_chars = "\\/:*?\"<>|";
						for (char &c: filename_base)
							if (invalid_fs_chars.find(c) != std::string::npos)
								c = '_';
						if (filename_base.length() > 150)
							filename_base = filename_base.substr(0, 150);
						if (filename_base.empty() || filename_base == "." || filename_base == "..") {
							std::hash<std::string> h;
							filename_base = "article_" + page_id_str + "_" + std::to_string(h(original_api_title_str));
							if (filename_base.length() > 150)
								filename_base = filename_base.substr(0, 150);
						}

						std::string filename_with_ext_str = filename_base + ".txt";
						std::string full_path_str = ARTICLES_DIR + filename_with_ext_str;

						std::ofstream outfile_stream;
#ifdef _WIN32
						outfile_stream.open(utf8_to_wstring_win(full_path_str).c_str(),
											std::ios::out | std::ios::binary);
#else
						outfile_stream.open(full_path_str, std::ios::out | std::ios::binary);
#endif

						if (outfile_stream.is_open()) {
							outfile_stream << article_extract_str;
							outfile_stream.close();
							session_metadata_output.push_back({filename_with_ext_str, original_api_title_str});
							articles_saved_this_session_count++;
							total_articles_ever_fetched_display_counter++;
							std::cout << "Saved (" << total_articles_ever_fetched_display_counter
									  << " total / this session: " << articles_saved_this_session_count << "/"
									  << num_articles_to_fetch_this_session << "): " << original_api_title_str << " -> "
									  << filename_with_ext_str << std::endl;
						} else {
							std::cerr << "Could not open file to save: " << full_path_str << std::endl;
						}
					}
					if (articles_saved_this_session_count >= num_articles_to_fetch_this_session)
						break;
				}
			} else {
				std::cerr << "Warning: JSON response malformed." << std::endl;
				if (readBuffer.length() < 1000)
					std::cerr << "Resp: " << readBuffer << std::endl;
			}
		} catch (const json::parse_error &e) {
			std::cerr << "JSON parse error: " << e.what() << std::endl;
		}

		if (articles_saved_this_session_count >= num_articles_to_fetch_this_session)
			break;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	std::cout << "Finished fetching for this session. Articles saved in this session: "
			  << articles_saved_this_session_count << std::endl;
	return articles_saved_this_session_count;
}

std::vector<std::string> tokenize(const std::string &text) {
	std::vector<std::string> tokens;
	std::string current_token;
	for (char ch_raw: text) {
		unsigned char ch = static_cast<unsigned char>(ch_raw);
		if (std::isalnum(ch)) {
			current_token += static_cast<char>(std::tolower(ch));
		} else {
			if (!current_token.empty()) {
				tokens.push_back(current_token);
				current_token.clear();
			}
		}
	}
	if (!current_token.empty()) {
		tokens.push_back(current_token);
	}
	return tokens;
}

const std::set<std::string> STOP_WORDS = {
		"a",		 "an",		 "the",		 "is",			 "are",		   "was",		 "were",	  "be",
		"been",		 "being",	 "have",	 "has",			 "had",		   "do",		 "does",	  "did",
		"will",		 "would",	 "should",	 "can",			 "could",	   "may",		 "might",	  "must",
		"and",		 "but",		 "or",		 "if",			 "because",	   "as",		 "until",	  "while",
		"of",		 "at",		 "by",		 "for",			 "with",	   "about",		 "against",	  "between",
		"into",		 "through",	 "during",	 "before",		 "after",	   "above",		 "below",	  "to",
		"from",		 "up",		 "down",	 "in",			 "out",		   "on",		 "off",		  "over",
		"under",	 "again",	 "further",	 "then",		 "once",	   "here",		 "there",	  "when",
		"where",	 "why",		 "how",		 "all",			 "any",		   "both",		 "each",	  "few",
		"more",		 "most",	 "other",	 "some",		 "such",	   "no",		 "nor",		  "not",
		"only",		 "own",		 "same",	 "so",			 "than",	   "too",		 "very",	  "s",
		"t",		 "just",	 "don",		 "now",			 "this",	   "that",		 "these",	  "those",
		"i",		 "you",		 "he",		 "she",			 "it",		   "we",		 "they",	  "me",
		"him",		 "her",		 "us",		 "them",		 "my",		   "your",		 "his",		  "its",
		"our",		 "their",	 "mine",	 "yours",		 "hers",	   "ours",		 "theirs",	  "myself",
		"yourself",	 "himself",	 "herself",	 "itself",		 "ourselves",  "themselves", "what",	  "which",
		"who",		 "whom",	 "also",	 "however",		 "could",	   "page",		 "wikipedia", "article",
		"articles",	 "edit",	 "source",	 "see",			 "references", "links",		 "external",  "history",
		"retrieved", "archived", "original", "isbn",		 "doi",		   "january",	 "february",  "march",
		"april",	 "may",		 "june",	 "july",		 "august",	   "september",	 "october",	  "november",
		"december",	 "com",		 "org",		 "html",		 "http",	   "https",		 "www",		  "also",
		"often",	 "used",	 "many",	 "one",			 "two",		   "first",		 "second",	  "new",
		"old",		 "year",	 "years",	 "time",		 "day",		   "days",		 "like",	  "example",
		"state",	 "states",	 "city",	 "world",		 "part",	   "system",	 "group",	  "number",
		"form",		 "such as",	 "known as", "according to", "as well as"};

std::vector<std::string> remove_stopwords(const std::vector<std::string> &tokens) {
	std::vector<std::string> filtered_tokens;
	filtered_tokens.reserve(tokens.size());
	for (const auto &token: tokens) {
		if (STOP_WORDS.find(token) == STOP_WORDS.end() && token.length() >= 3 && token.length() < 25) {
			bool all_digits = true;
			for (char c: token) {
				if (!std::isdigit(static_cast<unsigned char>(c))) {
					all_digits = false;
					break;
				}
			}
			if (!all_digits) {
				filtered_tokens.push_back(token);
			}
		}
	}
	return filtered_tokens;
}

void load_documents_from_dir(const std::string &dir_path,
							 std::vector<std::pair<std::string, std::string>> &documents_data, int max_docs_to_load) {
	documents_data.clear();
	int count = 0;
	if (!std::filesystem::exists(dir_path)) {
		std::cerr << "Error: Directory for loading documents " << dir_path << " does not exist." << std::endl;
		return;
	}
	try {
		for (const auto &entry: std::filesystem::directory_iterator(dir_path)) {
			if (max_docs_to_load > 0 && count >= max_docs_to_load)
				break;
			if (entry.is_regular_file() && entry.path().extension() == ".txt") {
				std::ifstream file_stream;
#ifdef _WIN32
				file_stream.open(utf8_to_wstring_win(entry.path().string()).c_str(), std::ios::in | std::ios::binary);
#else
				file_stream.open(entry.path().string(), std::ios::in | std::ios::binary);
#endif

				if (file_stream.is_open()) {
					std::stringstream buffer;
					buffer << file_stream.rdbuf();
					documents_data.push_back({entry.path().filename().string(), buffer.str()});
					file_stream.close();
					count++;
				} else {
					std::cerr << "Could not open file for reading: " << entry.path() << std::endl;
				}
			}
		}
	} catch (const std::filesystem::filesystem_error &e) {
		std::cerr << "Filesystem error while iterating directory " << dir_path << ": " << e.what() << std::endl;
	}
}

void save_sparse_matrix_binary(const Eigen::SparseMatrix<double> &matrix, const std::string &filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!out.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for writing sparse matrix." << std::endl;
		return;
	}
	Eigen::Index rows = matrix.rows();
	Eigen::Index cols = matrix.cols();
	Eigen::Index nonZeros = matrix.nonZeros();
	out.write(reinterpret_cast<const char *>(&rows), sizeof(Eigen::Index));
	out.write(reinterpret_cast<const char *>(&cols), sizeof(Eigen::Index));
	out.write(reinterpret_cast<const char *>(&nonZeros), sizeof(Eigen::Index));
	for (int k = 0; k < matrix.outerSize(); ++k) {
		for (Eigen::SparseMatrix<double>::InnerIterator it(matrix, k); it; ++it) {
			Eigen::Index r = it.row();
			Eigen::Index c = it.col();
			double val = it.value();
			out.write(reinterpret_cast<const char *>(&r), sizeof(Eigen::Index));
			out.write(reinterpret_cast<const char *>(&c), sizeof(Eigen::Index));
			out.write(reinterpret_cast<const char *>(&val), sizeof(double));
		}
	}
	out.close();
	std::cout << "Saved sparse matrix to " << filename << " (" << rows << "x" << cols << ", " << nonZeros
			  << " non-zeros)" << std::endl;
}

void save_vector_binary(const Eigen::VectorXd &vector, const std::string &filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!out.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for writing vector." << std::endl;
		return;
	}
	Eigen::Index size = vector.size();
	out.write(reinterpret_cast<const char *>(&size), sizeof(Eigen::Index));
	out.write(reinterpret_cast<const char *>(vector.data()), size * sizeof(typename Eigen::VectorXd::Scalar));
	out.close();
	std::cout << "Saved vector to " << filename << " (size " << size << ")" << std::endl;
}

void save_dense_matrix_binary(const Eigen::MatrixXd &matrix, const std::string &filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!out.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for writing dense matrix." << std::endl;
		return;
	}
	Eigen::Index rows = matrix.rows();
	Eigen::Index cols = matrix.cols();
	out.write(reinterpret_cast<const char *>(&rows), sizeof(Eigen::Index));
	out.write(reinterpret_cast<const char *>(&cols), sizeof(Eigen::Index));
	out.write(reinterpret_cast<const char *>(matrix.data()), rows * cols * sizeof(typename Eigen::MatrixXd::Scalar));
	out.close();
	std::cout << "Saved dense matrix to " << filename << " (" << rows << "x" << cols << ")" << std::endl;
}

bool perform_sparse_svd_spectra(const Eigen::SparseMatrix<double> &A, int k_target, Eigen::MatrixXd &U_k_out,
								Eigen::VectorXd &S_k_diag_out, Eigen::MatrixXd &V_k_out) {
	if (A.rows() == 0 || A.cols() == 0 || k_target <= 0) {
		std::cerr << "SPECTRA SVD Error: Invalid input matrix or k_target." << std::endl;
		return false;
	}
	int m = A.rows();
	int n = A.cols();
	if (m <= 1 || n <= 1) {
		std::cerr << "SPECTRA SVD Error: Matrix dimensions too small (m=" << m << ", n=" << n << ")." << std::endl;
		return false;
	}

	int actual_k = std::min({(long) m - 1L, (long) n - 1L, (long) k_target});
	if (actual_k <= 0) {
		std::cerr << "SPECTRA SVD Error: actual_k (" << actual_k << ") is <= 0. Must be > 0 for SVD." << std::endl;
		return false;
	}

	int ncv = std::min({(long) m, (long) n, (long) (2 * actual_k + 5)});
	if (ncv <= actual_k)
		ncv = actual_k + 2;
	if (ncv >= std::min(m, n))
		ncv = std::min(m, n) - 1;
	if (ncv <= 0)
		ncv = std::min({(long) m, (long) n, (long) 3L, (long) actual_k + 1L});
	if (actual_k >= ncv && ncv > 0)
		actual_k = ncv - 1;
	if (actual_k <= 0 || ncv <= 0) {
		std::cerr << "SPECTRA SVD Error: Could not determine valid actual_k (" << actual_k << ") and ncv (" << ncv
				  << "). Min dim: " << std::min(m, n) << std::endl;
		return false;
	}

	std::cout << "Spectra SVD: Attempting with k=" << actual_k << " and ncv=" << ncv << std::endl;

	try {
		Spectra::CompInfo info;
		if (m >= n) {
			Eigen::SparseMatrix<double> AtA = A.transpose() * A;
			if (AtA.rows() == 0 || actual_k <= 0 || ncv <= actual_k || actual_k >= AtA.rows() || ncv >= AtA.rows()) {
				std::cerr << "SPECTRA SVD (AtA) Error: Invalid k/ncv for AtA." << std::endl;
				return false;
			}
			Spectra::SparseSymMatProd<double> op(AtA);
			Spectra::SymEigsSolver<Spectra::SparseSymMatProd<double>> eigs(op, actual_k, ncv);
			eigs.init();
			eigs.compute(Spectra::SortRule::LargestMagn, 1000, 1e-10);
			info = eigs.info();
			if (info != Spectra::CompInfo::Successful) {
				std::cerr << "SPECTRA SVD (AtA) Error: Decomposition failed. Info: " << static_cast<int>(info)
						  << std::endl;
				return false;
			}
			Eigen::VectorXd eigenvalues = eigs.eigenvalues();
			V_k_out = eigs.eigenvectors();
			S_k_diag_out.resize(actual_k);
			for (int i = 0; i < actual_k; ++i)
				S_k_diag_out(i) = (eigenvalues(i) > 1e-12) ? std::sqrt(eigenvalues(i)) : 0.0;
			Eigen::VectorXd S_k_inv(actual_k);
			for (int i = 0; i < actual_k; ++i)
				S_k_inv(i) = (S_k_diag_out(i) > 1e-9) ? 1.0 / S_k_diag_out(i) : 0.0;
			U_k_out = A * (V_k_out * S_k_inv.asDiagonal());
		} else {
			Eigen::SparseMatrix<double> AAt = A * A.transpose();
			if (AAt.rows() == 0 || actual_k <= 0 || ncv <= actual_k || actual_k >= AAt.rows() || ncv >= AAt.rows()) {
				std::cerr << "SPECTRA SVD (AAt) Error: Invalid k/ncv for AAt." << std::endl;
				return false;
			}
			Spectra::SparseSymMatProd<double> op(AAt);
			Spectra::SymEigsSolver<Spectra::SparseSymMatProd<double>> eigs(op, actual_k, ncv);
			eigs.init();
			eigs.compute(Spectra::SortRule::LargestMagn, 1000, 1e-10);
			info = eigs.info();
			if (info != Spectra::CompInfo::Successful) {
				std::cerr << "SPECTRA SVD (AAt) Error: Decomposition failed. Info: " << static_cast<int>(info)
						  << std::endl;
				return false;
			}
			Eigen::VectorXd eigenvalues = eigs.eigenvalues();
			U_k_out = eigs.eigenvectors();
			S_k_diag_out.resize(actual_k);
			for (int i = 0; i < actual_k; ++i)
				S_k_diag_out(i) = (eigenvalues(i) > 1e-12) ? std::sqrt(eigenvalues(i)) : 0.0;
			Eigen::VectorXd S_k_inv(actual_k);
			for (int i = 0; i < actual_k; ++i)
				S_k_inv(i) = (S_k_diag_out(i) > 1e-9) ? 1.0 / S_k_diag_out(i) : 0.0;
			V_k_out = A.transpose() * (U_k_out * S_k_inv.asDiagonal());
		}
	} catch (const std::exception &e) {
		std::cerr << "SPECTRA SVD Exception: " << e.what() << std::endl;
		return false;
	}
	std::cout << "Spectra SVD computation successful for actual k=" << actual_k << std::endl;
	return true;
}

int main(int argc, char *argv[]) {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);
	std::cout << std::fixed << std::setprecision(4);

	int num_articles_target_total = DEFAULT_NUM_ARTICLES_TO_FETCH;
	bool perform_svd_here = false;
	int k_dim_svd_target = 100;

	if (argc > 1) {
		try {
			num_articles_target_total = std::stoi(argv[1]);
			if (num_articles_target_total <= 0)
				num_articles_target_total = DEFAULT_NUM_ARTICLES_TO_FETCH;
		} catch (const std::exception &e) {
			std::cerr << "Invalid num_articles arg. Using default." << std::endl;
		}
	}
	for (int i = 2; i < argc; ++i) {
		std::string arg_str = argv[i];
		if (arg_str == "--svd") {
			perform_svd_here = true;
			if (i + 1 < argc) {
				try {
					k_dim_svd_target = std::stoi(argv[i + 1]);
					if (k_dim_svd_target <= 0)
						k_dim_svd_target = 100;
					i++;
				} catch (const std::exception &e) {
					std::cerr << "Warning: Could not parse k for SVD. Using default." << std::endl;
				}
			} else {
				std::cout << "Info: --svd flag, no k value. Using default k." << std::endl;
			}
		}
	}
	std::cout << "Targeting to have up to " << num_articles_target_total << " articles for processing." << std::endl;
	if (perform_svd_here)
		std::cout << "SVD requested with target k=" << k_dim_svd_target << "." << std::endl;

	std::vector<FetchedArticleMeta> all_docs_metadata;
	std::string metadata_filename = "doc_metadata.tsv";

	if (std::filesystem::exists(metadata_filename)) {
		QFile meta_qfile_in(QString::fromStdString(metadata_filename));
		if (meta_qfile_in.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream meta_in_stream(&meta_qfile_in);
			meta_in_stream.setEncoding(QStringConverter::Utf8);
			while (!meta_in_stream.atEnd()) {
				QString q_line = meta_in_stream.readLine();
				std::string line_std = q_line.toStdString();
				std::stringstream ss_line(line_std);
				std::string disk_name_ext, original_title;
				if (std::getline(ss_line, disk_name_ext, '\t') && std::getline(ss_line, original_title)) {
					if (std::filesystem::exists(ARTICLES_DIR + disk_name_ext)) {
						all_docs_metadata.push_back({disk_name_ext, original_title});
					} else { /* ... log skipped ... */
					}
				}
			}
			meta_qfile_in.close();
			std::cout << "Loaded " << all_docs_metadata.size() << " entries from existing metadata file." << std::endl;
		} else {
			std::cerr << "Could not open existing metadata file." << std::endl;
		}
	}

	int articles_to_fetch_now = 0;
	if (static_cast<int>(all_docs_metadata.size()) < num_articles_target_total) {
		articles_to_fetch_now = num_articles_target_total - static_cast<int>(all_docs_metadata.size());
	}

	int total_fetched_counter_for_display = static_cast<int>(all_docs_metadata.size());
	if (articles_to_fetch_now > 0) {
		std::cout << "Need to fetch " << articles_to_fetch_now << " more." << std::endl;
		std::vector<FetchedArticleMeta> newly_fetched_this_session;
		fetch_and_save_random_wikipedia_articles(articles_to_fetch_now, newly_fetched_this_session,
												 total_fetched_counter_for_display);
		all_docs_metadata.insert(all_docs_metadata.end(), newly_fetched_this_session.begin(),
								 newly_fetched_this_session.end());

		QFile meta_qfile_out(QString::fromStdString(metadata_filename));
		if (meta_qfile_out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			QTextStream meta_out_stream(&meta_qfile_out);
			meta_out_stream.setEncoding(QStringConverter::Utf8);
			for (const auto &meta: all_docs_metadata) {
				meta_out_stream << QString::fromUtf8(meta.filename_on_disk_with_ext.c_str()) << "\t"
								<< QString::fromUtf8(meta.original_api_title.c_str()) << "\n";
			}
			meta_qfile_out.close();
			std::cout << "Updated metadata file with " << all_docs_metadata.size() << " entries." << std::endl;
		} else {
			std::cerr << "Error opening metadata file for writing." << std::endl;
		}
	} else {
		std::cout << "Sufficient articles from metadata. No new fetching." << std::endl;
	}

	if (num_articles_target_total > 0 && static_cast<int>(all_docs_metadata.size()) > num_articles_target_total) {
		all_docs_metadata.resize(num_articles_target_total);
		std::cout << "Metadata list truncated to " << num_articles_target_total << " articles." << std::endl;
	}
	if (all_docs_metadata.empty()) {
		std::cerr << "No document metadata. Exiting." << std::endl;
		return 1;
	}

	int n_docs = static_cast<int>(all_docs_metadata.size());
	std::vector<std::pair<std::string, std::string>> documents_content_data;
	documents_content_data.reserve(n_docs);
	std::vector<std::string> final_doc_original_titles(n_docs);
	std::vector<std::string> final_doc_disk_filenames(n_docs);

	std::cout << "Loading content for " << n_docs << " documents..." << std::endl;
	int valid_docs_loaded_count = 0;
	for (const auto &meta: all_docs_metadata) {
		std::string full_path_to_article = ARTICLES_DIR + meta.filename_on_disk_with_ext;
		std::ifstream file_content_stream;
#ifdef _WIN32
		file_content_stream.open(utf8_to_wstring_win(full_path_to_article).c_str(), std::ios::in | std::ios::binary);
#else
		file_content_stream.open(full_path_to_article, std::ios::in | std::ios::binary);
#endif
		if (file_content_stream.is_open()) {
			std::stringstream buffer;
			buffer << file_content_stream.rdbuf();
			documents_content_data.push_back({meta.filename_on_disk_with_ext, buffer.str()});
			final_doc_original_titles[valid_docs_loaded_count] = meta.original_api_title;
			final_doc_disk_filenames[valid_docs_loaded_count] = meta.filename_on_disk_with_ext;
			file_content_stream.close();
			valid_docs_loaded_count++;
		} else {
			std::cerr << "Error: Could not load content for " << meta.filename_on_disk_with_ext << ". Skipped."
					  << std::endl;
		}
	}
	n_docs = valid_docs_loaded_count;
	final_doc_original_titles.resize(n_docs);
	final_doc_disk_filenames.resize(n_docs);
	if (n_docs == 0) {
		std::cerr << "No document content loaded. Exiting." << std::endl;
		return 1;
	}
	std::cout << "Loaded content for " << n_docs << " documents." << std::endl;

	std::map<std::string, int> term_to_id;
	std::vector<std::string> id_to_term;
	std::vector<std::vector<std::string>> processed_docs_tokens(n_docs);
	std::cout << "Processing documents and building vocabulary..." << std::endl;
	for (int j = 0; j < n_docs; ++j) { /* ... implementacja jak w ostatniej pełnej wersji ... */
		if (j > 0 && j % 5000 == 0)
			std::cout << " Tokenized " << j << " docs..." << std::endl;
		std::vector<std::string> toks = tokenize(documents_content_data[j].second);
		processed_docs_tokens[j] = remove_stopwords(toks);
		for (const auto &tok: processed_docs_tokens[j]) {
			if (term_to_id.find(tok) == term_to_id.end()) {
				term_to_id[tok] = id_to_term.size();
				id_to_term.push_back(tok);
			}
		}
	}
	documents_content_data.clear();
	documents_content_data.shrink_to_fit();
	int m_terms = static_cast<int>(id_to_term.size());
	if (m_terms == 0) {
		std::cerr << "Vocabulary empty. Exiting." << std::endl;
		return 1;
	}
	std::cout << "Vocabulary size: " << m_terms << " terms." << std::endl;

	std::cout << "Building TF sparse matrix..." << std::endl;
	std::vector<Eigen::Triplet<double>> triplets_tf;
	triplets_tf.reserve(static_cast<size_t>(n_docs) * 200);
	for (int j = 0; j < n_docs; ++j) { /* ... implementacja jak w ostatniej pełnej wersji ... */
		if (j > 0 && j % 10000 == 0)
			std::cout << " TF trip doc " << j << std::endl;
		std::map<int, int> tc;
		try {
			for (const auto &t: processed_docs_tokens.at(j)) {
				auto itm = term_to_id.find(t);
				if (itm != term_to_id.end())
					tc[itm->second]++;
			}
		} catch (const std::out_of_range &e) {
		}
		for (const auto &p: tc)
			triplets_tf.push_back(Eigen::Triplet<double>(p.first, j, static_cast<double>(p.second)));
	}
	processed_docs_tokens.clear();
	processed_docs_tokens.shrink_to_fit();
	Eigen::SparseMatrix<double> A_tf_sparse(m_terms, n_docs);
	if (!triplets_tf.empty())
		A_tf_sparse.setFromTriplets(triplets_tf.begin(), triplets_tf.end());
	A_tf_sparse.makeCompressed();
	triplets_tf.clear();
	triplets_tf.shrink_to_fit();
	std::cout << "TF sparse matrix built. Non-zeros: " << A_tf_sparse.nonZeros() << std::endl;

	Eigen::VectorXd idf_vector = Eigen::VectorXd::Zero(m_terms);
	std::cout << "Calculating IDF vector..." << std::endl;
	Eigen::VectorXd doc_freq_vec = Eigen::VectorXd::Zero(m_terms);
	for (int k = 0; k < A_tf_sparse.outerSize(); ++k)
		for (Eigen::SparseMatrix<double>::InnerIterator it(A_tf_sparse, k); it; ++it)
			if (it.value() > 1e-9)
				doc_freq_vec(it.row()) += 1.0;
	for (int i = 0; i < m_terms; ++i)
		if (doc_freq_vec(i) > 0)
			idf_vector(i) = std::log(static_cast<double>(n_docs) / doc_freq_vec(i));
	std::cout << "IDF vector calculated." << std::endl;

	Eigen::SparseMatrix<double> A_tfidf_sparse = A_tf_sparse;
	std::cout << "Building TF-IDF sparse matrix..." << std::endl;
	for (int k = 0; k < A_tfidf_sparse.outerSize(); ++k)
		for (Eigen::SparseMatrix<double>::InnerIterator it(A_tfidf_sparse, k); it; ++it)
			it.valueRef() *= idf_vector(it.row());
	A_tfidf_sparse.prune(0.0, 1e-9);
	A_tfidf_sparse.makeCompressed();
	std::cout << "TF-IDF sparse matrix built. Non-zeros: " << A_tfidf_sparse.nonZeros() << std::endl;
	A_tf_sparse.resize(0, 0);
	if (A_tf_sparse.nonZeros() > 0)
		A_tf_sparse.data().squeeze();

	std::cout << "Saving processed data..." << std::endl;
	QFile vocab_qf_out(QString::fromStdString("vocabulary.txt"));
	if (vocab_qf_out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		QTextStream vos(&vocab_qf_out);
		vos.setEncoding(QStringConverter::Utf8);
		for (const auto &t: id_to_term)
			vos << QString::fromUtf8(t.c_str()) << "\n";
		vocab_qf_out.close();
		std::cout << "Saved vocab.txt" << std::endl;
	} else {
		std::cerr << "Err save vocab" << std::endl;
	}

	QFile diskfn_qf_out(QString::fromStdString("doc_disk_filenames.txt"));
	if (diskfn_qf_out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		QTextStream dfos(&diskfn_qf_out);
		dfos.setEncoding(QStringConverter::Utf8);
		for (const auto &n: final_doc_disk_filenames)
			dfos << QString::fromUtf8(n.c_str()) << "\n";
		diskfn_qf_out.close();
		std::cout << "Saved disk_filenames.txt" << std::endl;
	} else {
		std::cerr << "Err save disk_fn" << std::endl;
	}
	QFile origtit_qf_out(QString::fromStdString("doc_original_titles.txt"));
	if (origtit_qf_out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		QTextStream otos(&origtit_qf_out);
		otos.setEncoding(QStringConverter::Utf8);
		for (const auto &t: final_doc_original_titles)
			otos << QString::fromUtf8(t.c_str()) << "\n";
		origtit_qf_out.close();
		std::cout << "Saved original_titles.txt" << std::endl;
	} else {
		std::cerr << "Err save orig_tit" << std::endl;
	}

	save_sparse_matrix_binary(A_tfidf_sparse, "tdm_tfidf_sparse.dat");
	save_vector_binary(idf_vector, "idf_vector.dat");

	if (perform_svd_here) {
		std::cout << "Attempting SVD (target k=" << k_dim_svd_target << ") using Spectra..." << std::endl;
		if (m_terms > 1 && n_docs > 1 && A_tfidf_sparse.nonZeros() > 0 && k_dim_svd_target > 0) {
			Eigen::MatrixXd Uk_dense;
			Eigen::VectorXd Sk_diag_k_vec;
			Eigen::MatrixXd Vk_dense;
			if (perform_sparse_svd_spectra(A_tfidf_sparse, k_dim_svd_target, Uk_dense, Sk_diag_k_vec, Vk_dense)) {
				save_dense_matrix_binary(Uk_dense, "U_k.dat");
				save_vector_binary(Sk_diag_k_vec, "S_k_diag.dat");
				save_dense_matrix_binary(Vk_dense, "V_k.dat");
			} else {
				std::cout << "Sparse SVD failed/skipped." << std::endl;
			}
		} else {
			std::cerr << "Cannot perform SVD. Conditions not met." << std::endl;
		}
	}

	std::cout << "Data processing complete." << std::endl;
	return 0;
}
