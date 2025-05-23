#include "searchenginecore.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringConverter>
#include <QTextStream>
#include <QUrl>

const std::set<std::string> SearchEngineCore::STOP_WORDS = {
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

Eigen::SparseMatrix<double> SearchEngineCore::load_eigen_sparse_matrix_binary(const std::string &filename) const {
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "Core Error: Could not open file " << filename << " for reading sparse matrix." << std::endl;
		return Eigen::SparseMatrix<double>(0, 0);
	}
	Eigen::Index rows = 0, cols = 0, nonZeros = 0;
	in.read(reinterpret_cast<char *>(&rows), sizeof(Eigen::Index));
	in.read(reinterpret_cast<char *>(&cols), sizeof(Eigen::Index));
	in.read(reinterpret_cast<char *>(&nonZeros), sizeof(Eigen::Index));

	if (rows < 0 || cols < 0 || nonZeros < 0 || rows > 2000000 || cols > 2000000 || nonZeros > 2000000000) {
		std::cerr << "Core Error: Invalid dimensions/nonZeros in sparse matrix file " << filename << " (r:" << rows
				  << ", c:" << cols << ", nz:" << nonZeros << ")" << std::endl;
		in.close();
		return Eigen::SparseMatrix<double>(0, 0);
	}
	std::vector<Eigen::Triplet<double>> triplets;
	if (nonZeros > 0)
		triplets.reserve(nonZeros);
	for (Eigen::Index i = 0; i < nonZeros; ++i) {
		Eigen::Index r_idx, c_idx;
		double val;
		in.read(reinterpret_cast<char *>(&r_idx), sizeof(Eigen::Index));
		in.read(reinterpret_cast<char *>(&c_idx), sizeof(Eigen::Index));
		in.read(reinterpret_cast<char *>(&val), sizeof(double));
		if (!in || (in.gcount() != sizeof(double) && i < nonZeros)) {
			std::cerr << "Core Error: Premature EOF or read error in sparse matrix file " << filename << " at non-zero "
					  << i << std::endl;
			return Eigen::SparseMatrix<double>(0, 0);
		}
		triplets.push_back(Eigen::Triplet<double>(r_idx, c_idx, val));
	}
	in.close();
	Eigen::SparseMatrix<double> matrix(rows, cols);
	if (!triplets.empty() || (rows >= 0 && cols >= 0 && nonZeros == 0)) {
		matrix.setFromTriplets(triplets.begin(), triplets.end());
	}
	std::cout << "Core: Loaded sparse matrix from " << filename << " (" << rows << "x" << cols << ", "
			  << matrix.nonZeros() << " non-zeros, expected " << nonZeros << ")" << std::endl;
	return matrix;
}

Eigen::MatrixXd SearchEngineCore::load_eigen_dense_matrix_binary(const std::string &filename) const {
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "Core Error: Could not open file " << filename << " for reading dense matrix." << std::endl;
		return Eigen::MatrixXd(0, 0);
	}
	Eigen::Index rows = 0, cols = 0;
	in.read(reinterpret_cast<char *>(&rows), sizeof(Eigen::Index));
	in.read(reinterpret_cast<char *>(&cols), sizeof(Eigen::Index));
	if (rows > 0 && cols > 0 && rows < 2000000 && cols < 2000000) {
		Eigen::MatrixXd matrix(rows, cols);
		in.read(reinterpret_cast<char *>(matrix.data()), rows * cols * sizeof(double));
		if (!in || (in.gcount() != rows * cols * sizeof(double) && (rows * cols > 0))) {
			std::cerr << "Core Error: Premature EOF or read error in dense matrix file " << filename << std::endl;
			return Eigen::MatrixXd(0, 0);
		}
		in.close();
		std::cout << "Core: Loaded dense matrix from " << filename << " (" << rows << "x" << cols << ")" << std::endl;
		return matrix;
	} else if (rows == 0 && cols == 0) {
		in.close();
		std::cout << "Core: Loaded empty dense matrix from " << filename << std::endl;
		return Eigen::MatrixXd(0, 0);
	} else {
		std::cerr << "Core Error: Invalid dimensions in dense matrix file " << filename << " (r:" << rows
				  << ", c:" << cols << ")" << std::endl;
		in.close();
		return Eigen::MatrixXd(0, 0);
	}
}

Eigen::VectorXd SearchEngineCore::load_eigen_vector_binary(const std::string &filename) const {
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "Core Error: Could not open file " << filename << " for reading vector." << std::endl;
		return Eigen::VectorXd(0);
	}
	Eigen::Index size = 0;
	in.read(reinterpret_cast<char *>(&size), sizeof(Eigen::Index));
	if (size > 0 && size < 2000000) {
		Eigen::VectorXd vector(size);
		in.read(reinterpret_cast<char *>(vector.data()), size * sizeof(double));
		if (!in || (in.gcount() != size * sizeof(double) && size > 0)) {
			std::cerr << "Core Error: Premature EOF or read error in vector file " << filename << std::endl;
			return Eigen::VectorXd(0);
		}
		in.close();
		std::cout << "Core: Loaded vector from " << filename << " (size " << size << ")" << std::endl;
		return vector;
	} else if (size == 0) {
		in.close();
		std::cout << "Core: Loaded empty vector from " << filename << std::endl;
		return Eigen::VectorXd(0);
	} else {
		std::cerr << "Core Error: Invalid size in vector file " << filename << " (size=" << size << ")" << std::endl;
		in.close();
		return Eigen::VectorXd(0);
	}
}

SearchEngineCore::SearchEngineCore() :
	data_loaded_flag(false), svd_data_loaded_flag(false), m_terms_count(0), n_docs_count(0), k_svd_dim(0) {}

SearchEngineCore::~SearchEngineCore() {}

bool SearchEngineCore::loadData(const std::string &dataDirectory) {
	data_loaded_flag = false;
	svd_data_loaded_flag = false;
	id_to_term_vocab.clear();
	term_to_id_map.clear();
	doc_disk_filenames_list.clear();
	doc_original_titles_list.clear();
	A_tfidf_matrix_sparse.resize(0, 0);
	idf_terms_vector.resize(0);
	U_k_matrix.resize(0, 0);
	S_k_diag_vector.resize(0);
	V_k_matrix.resize(0, 0);
	D_k_docs_projected_matrix.resize(0, 0);

	std::cout << "Core: Attempting to load data from directory: " << dataDirectory << std::endl;

	std::string vocab_path = dataDirectory + "/vocabulary.txt";
	std::string disk_filenames_path = dataDirectory + "/doc_disk_filenames.txt";
	std::string original_titles_path = dataDirectory + "/doc_original_titles.txt";
	std::string tdm_sparse_path = dataDirectory + "/tdm_tfidf_sparse.dat";
	std::string idf_path = dataDirectory + "/idf_vector.dat";

	QFile vocab_qfile(QString::fromStdString(vocab_path));
	if (!vocab_qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cerr << "Core Error: Cannot open (Qt) vocabulary.txt at " << vocab_path << std::endl;
		return false;
	}
	QTextStream vocab_stream(&vocab_qfile);
	vocab_stream.setEncoding(QStringConverter::Utf8);
	while (!vocab_stream.atEnd()) {
		id_to_term_vocab.push_back(vocab_stream.readLine().toStdString());
	}
	vocab_qfile.close();
	if (id_to_term_vocab.empty()) {
		std::cerr << "Core Error: Vocabulary is empty!" << std::endl;
		return false;
	}
	for (size_t i = 0; i < id_to_term_vocab.size(); ++i) {
		term_to_id_map[id_to_term_vocab[i]] = static_cast<int>(i);
	}
	m_terms_count = static_cast<int>(id_to_term_vocab.size());
	std::cout << "Core: Loaded vocabulary (" << m_terms_count << " terms)." << std::endl;

	QFile disk_fn_qfile(QString::fromStdString(disk_filenames_path));
	if (!disk_fn_qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cerr << "Core Error: doc_disk_filenames.txt not found at " << disk_filenames_path << std::endl;
		return false;
	}
	QTextStream disk_fn_stream(&disk_fn_qfile);
	disk_fn_stream.setEncoding(QStringConverter::Utf8);
	while (!disk_fn_stream.atEnd()) {
		doc_disk_filenames_list.push_back(disk_fn_stream.readLine().toStdString());
	}
	disk_fn_qfile.close();
	if (doc_disk_filenames_list.empty()) {
		std::cerr << "Core Error: Disk filenames list is empty!" << std::endl;
		return false;
	}
	n_docs_count = static_cast<int>(doc_disk_filenames_list.size());
	std::cout << "Core: Loaded disk filenames (" << n_docs_count << " documents)." << std::endl;

	QFile orig_titles_qfile(QString::fromStdString(original_titles_path));
	if (!orig_titles_qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cerr << "Core Error: doc_original_titles.txt not found at " << original_titles_path << std::endl;
		return false;
	}
	QTextStream orig_titles_stream(&orig_titles_qfile);
	orig_titles_stream.setEncoding(QStringConverter::Utf8);
	while (!orig_titles_stream.atEnd()) {
		doc_original_titles_list.push_back(orig_titles_stream.readLine().toStdString());
	}
	orig_titles_qfile.close();
	if (doc_original_titles_list.empty() || doc_original_titles_list.size() != static_cast<size_t>(n_docs_count)) {
		std::cerr << "Core Error: Original titles list is empty or size mismatch with disk filenames! (Originals: "
				  << doc_original_titles_list.size() << ", DiskFilenames: " << n_docs_count << ")" << std::endl;
		return false;
	}
	std::cout << "Core: Loaded original API titles (" << doc_original_titles_list.size() << " titles)." << std::endl;

	A_tfidf_matrix_sparse = load_eigen_sparse_matrix_binary(tdm_sparse_path);
	idf_terms_vector = load_eigen_vector_binary(idf_path);

	if (A_tfidf_matrix_sparse.rows() != m_terms_count || A_tfidf_matrix_sparse.cols() != n_docs_count ||
		idf_terms_vector.size() != m_terms_count) {
		std::cerr << "Core Error: Mismatch in dimensions of loaded data matrices/vectors." << std::endl;
		std::cerr << "  A_tfidf_sparse: " << A_tfidf_matrix_sparse.rows() << "x" << A_tfidf_matrix_sparse.cols()
				  << " (expected " << m_terms_count << "x" << n_docs_count << ")" << std::endl;
		std::cerr << "  idf_terms_vector size: " << idf_terms_vector.size() << " (expected " << m_terms_count << ")"
				  << std::endl;
		return false;
	}
	data_loaded_flag = true;
	std::cout << "Core: Base data (sparse TF-IDF) loaded successfully." << std::endl;

	std::string uk_path = dataDirectory + "/U_k.dat";
	std::string sk_path = dataDirectory + "/S_k_diag.dat";
	std::string vk_path = dataDirectory + "/V_k.dat";

	if (std::filesystem::exists(uk_path) && std::filesystem::exists(sk_path) && std::filesystem::exists(vk_path)) {
		U_k_matrix = load_eigen_dense_matrix_binary(uk_path);
		S_k_diag_vector = load_eigen_vector_binary(sk_path);
		V_k_matrix = load_eigen_dense_matrix_binary(vk_path);

		if (U_k_matrix.rows() == m_terms_count && S_k_diag_vector.size() > 0 &&
			static_cast<Eigen::Index>(S_k_diag_vector.size()) == U_k_matrix.cols() &&
			S_k_diag_vector.size() == V_k_matrix.cols() && V_k_matrix.rows() == n_docs_count) {
			k_svd_dim = static_cast<int>(S_k_diag_vector.size());
			D_k_docs_projected_matrix.resize(k_svd_dim, n_docs_count);
			D_k_docs_projected_matrix = S_k_diag_vector.asDiagonal() * V_k_matrix.transpose();
			svd_data_loaded_flag = true;
			std::cout << "Core: SVD components loaded and validated (k=" << k_svd_dim << ")." << std::endl;
		} else {
			std::cerr << "Core Warning: SVD files found but failed validation or are empty." << std::endl;
			U_k_matrix.resize(0, 0);
			S_k_diag_vector.resize(0);
			V_k_matrix.resize(0, 0);
			D_k_docs_projected_matrix.resize(0, 0);
			svd_data_loaded_flag = false;
		}
	} else {
		std::cout << "Core Info: SVD component files not found (U_k.dat, S_k_diag.dat, V_k.dat)." << std::endl;
		svd_data_loaded_flag = false;
	}
	return true;
}

bool SearchEngineCore::isDataLoaded() const { return data_loaded_flag; }
bool SearchEngineCore::isSvdDataLoaded() const { return svd_data_loaded_flag; }
int SearchEngineCore::getVocabularySize() const { return m_terms_count; }
int SearchEngineCore::getDocumentCount() const { return n_docs_count; }
int SearchEngineCore::getSvdKDimension() const { return k_svd_dim; }

std::vector<std::string> SearchEngineCore::tokenize(const std::string &text) const {
	std::vector<std::string> tokens;
	std::string ct;
	for (char cr: text) {
		unsigned char c = static_cast<unsigned char>(cr);
		if (std::isalnum(c))
			ct += static_cast<char>(std::tolower(c));
		else if (!ct.empty()) {
			tokens.push_back(ct);
			ct.clear();
		}
	}
	if (!ct.empty())
		tokens.push_back(ct);
	return tokens;
}
std::vector<std::string> SearchEngineCore::remove_stopwords(const std::vector<std::string> &tokens) const {
	std::vector<std::string> ft;
	for (const auto &t: tokens) {
		if (STOP_WORDS.find(t) == STOP_WORDS.end() && t.length() >= 3 && t.length() < 25) {
			bool ad = 1;
			for (char c: t)
				if (!std::isdigit(static_cast<unsigned char>(c))) {
					ad = 0;
					break;
				}
			if (!ad)
				ft.push_back(t);
		}
	}
	return ft;
}

std::string SearchEngineCore::constructWikipediaURL(const std::string &original_api_title) const {
	std::string title_with_underscores = original_api_title;
	std::replace(title_with_underscores.begin(), title_with_underscores.end(), ' ', '_');
	QString q_title_path_segment = QString::fromUtf8(title_with_underscores.c_str());
	QByteArray additionally_safe_chars_for_wiki = ":/(),'";
	QByteArray encoded_segment =
			QUrl::toPercentEncoding(q_title_path_segment, QByteArray(), additionally_safe_chars_for_wiki);
	return "https://en.wikipedia.org/wiki/" + std::string(encoded_segment.constData());
}

double SearchEngineCore::cosine_similarity_dense_sparse(const Eigen::VectorXd &dense_vec,
														const Eigen::SparseVector<double> &sparse_col_vec) const {
	if (dense_vec.size() != sparse_col_vec.size() || dense_vec.size() == 0) {
		return 0.0;
	}
	double dot_product = dense_vec.transpose() * sparse_col_vec;
	double norm1 = dense_vec.norm();
	double norm2 = sparse_col_vec.norm();
	if (norm1 < 1e-9 || norm2 < 1e-9)
		return 0.0;
	return dot_product / (norm1 * norm2);
}
double SearchEngineCore::cosine_similarity_dense_dense(const Eigen::VectorXd &v1, const Eigen::VectorXd &v2) const {
	if (v1.size() != v2.size() || v1.size() == 0) {
		return 0.0;
	}
	double norm1 = v1.norm();
	double norm2 = v2.norm();
	if (norm1 < 1e-9 || norm2 < 1e-9)
		return 0.0;
	return v1.dot(v2) / (norm1 * norm2);
}

std::vector<SearchResult> SearchEngineCore::performSearchQuery(const std::string &queryString, bool useLSI, int topK) {
	std::vector<SearchResult> final_results;
	if (!data_loaded_flag) {
		std::cerr << "Core Error: Data not loaded!" << std::endl;
		return final_results;
	}
	if (m_terms_count == 0 || n_docs_count == 0) {
		std::cout << "Core Info: No terms or documents loaded." << std::endl;
		return final_results;
	}

	std::vector<std::string> query_tokens = tokenize(queryString);
	query_tokens = remove_stopwords(query_tokens);
	if (query_tokens.empty()) {
		std::cout << "Core Info: Query empty after processing." << std::endl;
		return final_results;
	}

	Eigen::VectorXd q_tf(m_terms_count);
	q_tf.setZero();
	int known_query_terms = 0;
	for (const auto &token: query_tokens) {
		auto it_map = term_to_id_map.find(token);
		if (it_map != term_to_id_map.end()) {
			q_tf(it_map->second)++;
			known_query_terms++;
		}
	}
	if (known_query_terms == 0) {
		std::cout << "Core Info: No known terms in query." << std::endl;
		return final_results;
	}

	Eigen::VectorXd q_tfidf = q_tf;
	for (int i = 0; i < m_terms_count; ++i) {
		if (i < idf_terms_vector.size())
			q_tfidf(i) *= idf_terms_vector(i);
		else {
			q_tfidf(i) = 0;
			std::cerr << "Core Warning: IDF vector too short for term index " << i << std::endl;
		}
	}

	std::vector<SearchResult> raw_search_results;
	raw_search_results.reserve(static_cast<size_t>(n_docs_count));

	bool lsi_attempted_and_failed = false;
	if (useLSI && svd_data_loaded_flag) {
		std::cout << "Core: Performing LSI search (k=" << k_svd_dim << ")" << std::endl;
		if (U_k_matrix.rows() != m_terms_count || U_k_matrix.cols() != k_svd_dim ||
			D_k_docs_projected_matrix.cols() != n_docs_count || D_k_docs_projected_matrix.rows() != k_svd_dim) {
			std::cerr << "Core Error: SVD matrices dimensions mismatch for LSI query. U_k:" << U_k_matrix.rows() << "x"
					  << U_k_matrix.cols() << " (exp " << m_terms_count << "x" << k_svd_dim << "), "
					  << "D_k_docs:" << D_k_docs_projected_matrix.rows() << "x" << D_k_docs_projected_matrix.cols()
					  << " (exp " << k_svd_dim << "x" << n_docs_count << ")" << std::endl;
			lsi_attempted_and_failed = true;
		} else {
			Eigen::VectorXd q_k = U_k_matrix.transpose() * q_tfidf;
			for (int j = 0; j < n_docs_count; ++j) {
				Eigen::VectorXd dj_k = D_k_docs_projected_matrix.col(j);
				double sim = cosine_similarity_dense_dense(q_k, dj_k);
				if (j < static_cast<int>(doc_disk_filenames_list.size()) &&
					j < static_cast<int>(doc_original_titles_list.size())) {
					raw_search_results.push_back({doc_disk_filenames_list.at(j), doc_original_titles_list.at(j),
												  constructWikipediaURL(doc_original_titles_list.at(j)), sim, j});
				}
			}
		}
	}

	if (!useLSI || !svd_data_loaded_flag || lsi_attempted_and_failed) {
		if (useLSI && !svd_data_loaded_flag) {
			std::cout << "Core Warning: LSI requested but SVD data not loaded. Using TF-IDF." << std::endl;
		} else if (useLSI && svd_data_loaded_flag && lsi_attempted_and_failed) {
			std::cout << "Core Info: LSI search failed due to matrix mismatch, using TF-IDF." << std::endl;
		}

		std::cout << "Core: Performing TF-IDF search (using sparse matrix)." << std::endl;
		raw_search_results.clear();
		for (int j = 0; j < A_tfidf_matrix_sparse.cols(); ++j) {
			if (j >= n_docs_count)
				break;
			Eigen::SparseVector<double> doc_sparse_col = A_tfidf_matrix_sparse.col(j);
			double sim = cosine_similarity_dense_sparse(q_tfidf, doc_sparse_col);
			if (j < static_cast<int>(doc_disk_filenames_list.size()) &&
				j < static_cast<int>(doc_original_titles_list.size())) {
				raw_search_results.push_back({doc_disk_filenames_list.at(j), doc_original_titles_list.at(j),
											  constructWikipediaURL(doc_original_titles_list.at(j)), sim, j});
			}
		}
	}

	std::sort(raw_search_results.begin(), raw_search_results.end());

	for (int i = 0; i < std::min(topK, static_cast<int>(raw_search_results.size())); ++i) {
		final_results.push_back(raw_search_results[i]);
	}
	return final_results;
}
