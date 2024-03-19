#pragma once

#include "document.h"
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
        , current_time(0)
        , no_result_requests(0) {
    }

    // ������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        uint64_t  timestamp;
        size_t results;
    };
    std::deque<QueryResult> requests_; // ������� �������� ��� ��������� � �������� �����������
    const static int min_in_day_ = 1440; // ���������� ����� � ������ ��� ������� ����������
    const SearchServer& search_server_;
    uint64_t  current_time;
    int no_result_requests; // ���������� �������� ��� ����������� �� ��������� �����

    void RecordTheResult(size_t size);
};