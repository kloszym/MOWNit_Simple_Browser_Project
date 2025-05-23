#ifndef SEARCHENGINECORE_H
#define SEARCHENGINECORE_H

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <map>
#include <set>
#include <string>
#include <vector>

struct SearchResult {
	std::string disk_filename_with_ext;
	std::string display_title;
	std::string url;
	double similarity;
	int doc_index_in_matrix;

	bool operator<(const SearchResult &other) const { return similarity > other.similarity; }
};

class SearchEngineCore {
public:
	SearchEngineCore();
	~SearchEngineCore();

	bool loadData(const std::string &dataDirectory = ".");

	std::vector<SearchResult> performSearchQuery(const std::string &queryString, bool useLSI, int topK);

	bool isDataLoaded() const;
	bool isSvdDataLoaded() const;
	int getVocabularySize() const;
	int getDocumentCount() const;
	int getSvdKDimension() const;

private:
	std::vector<std::string> tokenize(const std::string &text) const;
	std::vector<std::string> remove_stopwords(const std::vector<std::string> &tokens) const;

	std::string constructWikipediaURL(const std::string &original_api_title) const;

	double cosine_similarity_dense_sparse(const Eigen::VectorXd &dense_vec,
										  const Eigen::SparseVector<double> &sparse_col_vec) const;
	double cosine_similarity_dense_dense(const Eigen::VectorXd &v1, const Eigen::VectorXd &v2) const;

	std::vector<std::string> id_to_term_vocab;
	std::map<std::string, int> term_to_id_map;

	std::vector<std::string> doc_disk_filenames_list;
	std::vector<std::string> doc_original_titles_list;

	Eigen::SparseMatrix<double> A_tfidf_matrix_sparse;
	Eigen::VectorXd idf_terms_vector;

	Eigen::MatrixXd U_k_matrix;
	Eigen::VectorXd S_k_diag_vector;
	Eigen::MatrixXd V_k_matrix;
	Eigen::MatrixXd D_k_docs_projected_matrix;

	bool data_loaded_flag = false;
	bool svd_data_loaded_flag = false;
	int m_terms_count = 0;
	int n_docs_count = 0;
	int k_svd_dim = 0;

	static const std::set<std::string> STOP_WORDS;

	Eigen::SparseMatrix<double> load_eigen_sparse_matrix_binary(const std::string &filename) const;
	Eigen::MatrixXd load_eigen_dense_matrix_binary(const std::string &filename) const;
	Eigen::VectorXd load_eigen_vector_binary(const std::string &filename) const;
};

#endif // SEARCHENGINECORE_H
