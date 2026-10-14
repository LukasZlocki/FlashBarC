#include "SearchApps.h"

#include <algorithm>
#include <cctype>
#include <tuple>

namespace
{
std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

// Lower tier and position sort first; tier 0 is exact, 1 prefix, 2 word start, 3 inside a word.
struct MatchRank
{
    int tier = 0;
    std::size_t position = 0;
    std::size_t nameLength = 0;
};

bool operator<(const MatchRank &left, const MatchRank &right)
{
    return std::tie(left.tier, left.position, left.nameLength) < std::tie(right.tier, right.position, right.nameLength);
}

bool isWordSeparator(char character)
{
    return character == ' ' || character == '-' || character == '_' || character == '.' || character == '/';
}

MatchRank rankMatch(const std::string &normalizedName, const std::string &normalizedQuery, std::size_t position)
{
    if (normalizedQuery.empty()) {
        return {};
    }

    MatchRank rank;
    rank.position = position;
    rank.nameLength = normalizedName.size();
    if (normalizedName == normalizedQuery) {
        rank.tier = 0;
    } else if (position == 0) {
        rank.tier = 1;
    } else if (isWordSeparator(normalizedName[position - 1])) {
        rank.tier = 2;
    } else {
        rank.tier = 3;
    }
    return rank;
}

std::vector<domain::AppEntry> rankMatches(const std::vector<domain::AppEntry> &candidates, const std::string &normalizedQuery)
{
    struct RankedApp
    {
        MatchRank rank;
        const domain::AppEntry *appEntry;
    };

    std::vector<RankedApp> rankedApps;
    for (const domain::AppEntry &appEntry : candidates) {
        const std::string normalizedName = lowercase(appEntry.name);
        const std::size_t position = normalizedName.find(normalizedQuery);
        if (position != std::string::npos) {
            rankedApps.push_back({rankMatch(normalizedName, normalizedQuery, position), &appEntry});
        }
    }

    std::stable_sort(rankedApps.begin(), rankedApps.end(), [](const RankedApp &left, const RankedApp &right) {
        return left.rank < right.rank;
    });

    std::vector<domain::AppEntry> matchingApps;
    matchingApps.reserve(rankedApps.size());
    for (const RankedApp &rankedApp : rankedApps) {
        matchingApps.push_back(*rankedApp.appEntry);
    }
    return matchingApps;
}
}

SearchApps::SearchApps(const AppCatalogRepository &appCatalogRepository)
    : apps_(appCatalogRepository.loadApps())
{
}

void SearchApps::setDiscoveredApps(std::vector<domain::AppEntry> discoveredApps)
{
    discoveredApps_.clear();
    for (domain::AppEntry &discoveredApp : discoveredApps) {
        const bool alreadyInCatalog = std::any_of(apps_.begin(), apps_.end(),
            [&discoveredApp](const domain::AppEntry &appEntry) {
                return appEntry.command == discoveredApp.command
                    || (!discoveredApp.path.empty() && appEntry.path == discoveredApp.path);
            });
        if (!alreadyInCatalog) {
            discoveredApps_.push_back(std::move(discoveredApp));
        }
    }
}

std::vector<domain::AppEntry> SearchApps::execute(const std::string &query) const
{
    const std::string normalizedQuery = lowercase(query);
    std::vector<domain::AppEntry> matchingApps = rankMatches(apps_, normalizedQuery);
    if (matchingApps.empty()) {
        matchingApps = rankMatches(discoveredApps_, normalizedQuery);
    }
    return matchingApps;
}
