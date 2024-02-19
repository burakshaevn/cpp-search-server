#include <algorithm> 
#include <cassert>
#include <cmath> 
#include <iostream> 
#include <map> 
#include <numeric> 
#include <optional>
#include <set> 
#include <string> 
#include <utility> 
#include <vector> 

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int number;
    cin >> number;
    cin >> ws;
    return number;
}

enum class DocumentStatus
{
    ACTUAL, // актуальный 
    IRRELEVANT, // устаревший 
    BANNED, // отклонённый 
    REMOVED // удалённый 
};

// токенизация строки на слова (слова могут повторяться)
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    //Document() = default;

    Document() : id(0), relevance(0.0), rating(0), status_(DocumentStatus::ACTUAL)
    {}

    Document(int id, double relevance, int rating_) {
        this->id = id;
        this->relevance = relevance;
        this->rating = rating_;
        this->status_ = DocumentStatus::ACTUAL;
    }
    Document(int id, double relevance, int rating_, DocumentStatus status_) {
        this->id = id;
        this->relevance = relevance;
        this->rating = rating_;
        this->status_ = status_;
    }

    int id;
    double relevance;
    int rating;
    DocumentStatus status_;
};

class SearchServer {
public:

    SearchServer() = default;

    explicit SearchServer(const string& container) : SearchServer(set<string>{container})
    {}

    template<typename ContainerType>
    explicit  SearchServer(const ContainerType& container) {
        set<string> set_stop_words;
        for (const auto& stop_word : container) {
            if (!stop_word.empty()) {
                if (!IsValidWord(stop_word)) {
                    throw invalid_argument("Строка содержит недопустимые символы."s);
                }
                else {
                    set_stop_words.insert(stop_word);
                }
            }
            else continue;
        }
        for (const auto& stop_word : set_stop_words) {
            SetStopWords(stop_word);
        }
    }

    int GetDocumentId(int index) const {
        try {
            return documents_ids_.at(index);
        }
        catch (const out_of_range& error) {
            throw out_of_range("Значение id выходит за пределы [0, " + to_string(GetDocumentCount()) + ").");
        }
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& raw_query, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("Получено отрицательное значение id."s);
        }
        if (std::count(documents_ids_.begin(), documents_ids_.end(), document_id) > 0) {
            throw invalid_argument("Попытка добавить документ с существующим id."s);
        }
        const vector<string>& words = SplitIntoWordsNoStop(raw_query);
        const double temp = 1. / static_cast<double>(words.size()); 
        // TF - частота слова в конкретном документе
        for (const string& word : words) {
            words_freqs_[word][document_id] += temp;
        }
        documents_ids_.push_back(document_id);
        documents_[document_id].rating_ = ComputeAverageRating(ratings);
        documents_[document_id].status_ = status;
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query& query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, document_predicate); // returns: id, relevance, rating_, status_ 

        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (lhs.relevance != rhs.relevance) {
                return lhs.relevance > rhs.relevance;
            }
            else if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else return false;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_) const {
        return FindTopDocuments(raw_query, [&status_](const int document_id, const DocumentStatus t_status, const int rating_) {
            return status_ == t_status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) { // проходимся по плюс-словам 
            if (words_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (words_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return std::tie(matched_words, documents_.at(document_id).status_);
    }

private:
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct DocumentData {
        int rating_;
        DocumentStatus status_;
    };

    set<string> stop_words_;
    map<string, map<int, double>> words_freqs_; // returns: <word, map<id_document, relevance>>  
    map<int, DocumentData> documents_; // returns: <document_id, <rating_, status_>>   
    vector<int> documents_ids_;

    static bool IsValidWord(const string& word) { // Проверка на наличие спецсимволов
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static bool IsValidMinusWord(const string& word) { // Проверка на ненужный минус
        if ((word[0] == '-' and word[1] == '-') or word.empty()) {
            return false;
        }
        if (word == "-"s) {
            return false;
        }
        return true;
    }

    template <typename T>
    vector<Document> FindAllDocuments(const Query& query_words, T status) const {
        map<int, double> document_to_relevance; // <document_id, relevance> 
        for (const string& plus_word : query_words.plus_words) {
            if (words_freqs_.count(plus_word)) {
                for (auto& [document_id, tf] : words_freqs_.at(plus_word)) {
                    if (status(document_id, documents_.at(document_id).status_, documents_.at(document_id).rating_)) {
                        document_to_relevance[document_id] += ComputeIDF(plus_word) * tf;
                    }
                }
            }
        }
        for (const string& minus_word : query_words.minus_words) {
            if (words_freqs_.count(minus_word)) {
                for (const auto& [document_id, relevance] : words_freqs_.at(minus_word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
        vector<Document> matched_documents;
        for (auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating_, documents_.at(document_id).status_ });
        }
        return matched_documents;
    }

    static int ComputeAverageRating(const vector<int>& ratings_) {
        if (ratings_.empty()) return 0;
        else {
            return std::accumulate(ratings_.begin(), ratings_.end(), 0) / static_cast<int>(ratings_.size());
        }
    }

    double ComputeIDF(const string& plus_word) const {
        return log(static_cast<double>(documents_.size()) / words_freqs_.at(plus_word).size());
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Строка содержит недопустимые символы."s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const { // поиск по запросу 
        for (const auto& token : SplitIntoWords(text)) {
            if (!IsValidWord(token)) {
                throw invalid_argument("Присутствуют недопустимые символы с кодами [0, 31]."s);
            }
            if (!IsValidMinusWord(token)) {
                throw invalid_argument("Строка имеет некорректное расположение/количество минуса.");
            }
        }
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word.at(0) != '-') {
                query_words.plus_words.insert(word);
            }
            else {
                query_words.minus_words.insert(word.substr(1));
            }
        }
        return query_words;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    setlocale(LC_ALL, "RUS");
    SearchServer search_server("и в на"s);
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    try {
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        for (const Document& document : search_server.FindTopDocuments("--пушистый"s)) {
            PrintDocument(document);
        }
    }
    catch (const invalid_argument& error) {
        std::cout << error.what() << std::endl;
    }
}