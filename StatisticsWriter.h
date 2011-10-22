#ifndef SERVERVAULTSTATISTICS_WRITER_H
#define SERVERVAULTSTATISTICS_WRITER_H

#include "Precomp.h"

typedef std::map<std::string, int> StatisticPair;
typedef std::map<std::string, StatisticPair> StatisticMap;

typedef std::pair<int, std::string> ToplistPair;
typedef std::vector<ToplistPair> ToplistVec;
typedef std::map<std::string, ToplistVec> ToplistMap;

typedef std::vector<std::string> WarningVect;

class StatisticsWriter {
protected:
	std::string m_Filename;
	std::ofstream m_Log;
	WarningVect m_Warnings;

public:
	unsigned int ToplistMax;
	int Format;
	unsigned long CountedBics;
	unsigned long IgnoredBics;

	std::map<std::string, bool> WriteQuery;

	StatisticsWriter( std::string i_Filename ) {
		m_Filename = i_Filename;
		m_Log.open( m_Filename );
		Format = 0;
		ToplistMax = 10;
	}

	void LogWarning( std::string i_Warning ) {
		m_Warnings.push_back( i_Warning );
	}

	void WriteWarnings() {
		m_Log << "\n\nErrors / Warnings:";
		for ( WarningVect::iterator i = m_Warnings.begin(); i < m_Warnings.end(); i++ ) {
			m_Log << "\n" << i->c_str();
		}
	}

	void WriteStatistic( std::string i_Header, StatisticPair &i_Statistic ) {
		// Styles.
		std::string OutputHead;
		std::string OutputRow;
		std::string OutputFooter;
		if ( Format == 1 ) {
			OutputHead = "\n== %s ==\n{| class=\"wikitable sortable\" style=\"border-spacing: 7px; border-width: 0;\"\n! %s\n! Count\n! %%";
			OutputRow = "\n|-\n| %u || %s || %.2f%%";
			OutputFooter = "\n|}\n";
		} else {
			OutputHead = "\n[%s]";
			OutputRow = "\n%u - %s (%.2f%%)";
			OutputFooter = "\n";
			stringToLowerCase( i_Header );
		}

		// Write the statistic.
		if ( Format == 1 ) m_Log << boost::format( OutputHead ) % i_Header % i_Header;
		else m_Log << boost::format( OutputHead ) % i_Header;
		for ( StatisticPair::iterator i = i_Statistic.begin(); i != i_Statistic.end(); i++ ) {
			if ( i->second == 0 ) continue;
			if ( i->first.empty() ) continue;
			m_Log << boost::format( OutputRow ) % i->first % i->second % (double)( ( (double)i->second / (double)CountedBics ) * (double)100 );
		}
		m_Log << OutputFooter;
	}

	void WriteStatistics( StatisticMap &Statistics ) {
		// Get the style.
		std::string OutputHead;
		if ( Format == 1 ) {
			OutputHead = "\n= Statistics =";
		} else {
			OutputHead = "\nStatistics:";
		}
		m_Log << OutputHead;

		// Write statistics.
		if ( WriteQuery["gender"] ) WriteStatistic( "Gender", Statistics["gender"] );
		if ( WriteQuery["race"] ) WriteStatistic( "Race", Statistics["race"] );
		if ( WriteQuery["subrace"] ) WriteStatistic( "Subrace", Statistics["subrace"] );
		if ( WriteQuery["background"] ) WriteStatistic( "Background", Statistics["background"] );
		if ( WriteQuery["alignment"] ) WriteStatistic( "Alignment", Statistics["alignment"] );
		if ( WriteQuery["deity"] ) WriteStatistic( "Deity", Statistics["deity"] );
		if ( WriteQuery["levels"] ) WriteStatistic( "Class", Statistics["levels"] );
		if ( WriteQuery["skills"] ) WriteStatistic( "Skill", Statistics["skills"] );
		if ( WriteQuery["feats"] ) WriteStatistic( "Feat", Statistics["feats"] );
		if ( WriteQuery["tails"] ) WriteStatistic( "Tail", Statistics["tails"] );
		if ( WriteQuery["wings"] ) WriteStatistic( "Wing", Statistics["wings"] );
	}

	void WriteToplist( std::string i_Header, ToplistVec &i_Toplist, bool i_ReverseSort = false ) {
		// Styles.
		std::string OutputHead;
		std::string OutputRow;
		std::string OutputFooter;
		if ( Format == 1 ) {
			OutputHead = "\n== %s ==\n{| class=\"wikitable sortable\" style=\"border-spacing: 7px; border-width: 0;\"\n! %s\n! Character";
			OutputRow = "\n|-\n| %u || %s";
			OutputFooter = "\n|}\n";
		} else {
			OutputHead = "\n[%s]";
			OutputRow = "\n%u - %s";
			OutputFooter = "\n";
			stringToLowerCase( i_Header );
		}
		
		// Sort the toplist.
		if ( !i_ReverseSort ) std::sort( i_Toplist.rbegin(), i_Toplist.rend() );
		else std::sort( i_Toplist.begin(), i_Toplist.end() );

		// Write the toplist.
		if ( Format == 1 ) m_Log << boost::format( OutputHead ) % i_Header % i_Header;
		else m_Log << boost::format( OutputHead ) % i_Header;
		int c = 0;
		for ( ToplistVec::iterator i = i_Toplist.begin(); i < i_Toplist.end(); i++ ) {
			if ( c > ToplistMax ) break;
			if ( i->first == 0 ) continue;
			m_Log << boost::format( OutputRow ) % i->first % i->second;
			c++;
		}
		m_Log << OutputFooter;
	}

	void WriteToplists( ToplistMap &Toplists ) {
		if ( !WriteQuery["top"] ) return;

		// Get the style.
		std::string OutputHead;
		if ( Format == 1 ) {
			OutputHead = "\n= Toplists =";
		} else {
			OutputHead = "\nToplists:";
		}
		m_Log << OutputHead;

		// Write toplists.
		if ( WriteQuery["top-health"] ) WriteToplist( "Health", Toplists["health"] );
		if ( WriteQuery["top-armorclass"] ) WriteToplist( "Armor Class", Toplists["armorclass"] );
		if ( WriteQuery["top-baseattackbonus"] ) WriteToplist( "Base Attack Bonus", Toplists["baseattackbonus"] );
		if ( WriteQuery["top-abilities"] ) {
			WriteToplist( "Strength", Toplists["strength"] );
			WriteToplist( "Dexterity", Toplists["dexterity"] );
			WriteToplist( "Constitution", Toplists["constitution"] );
			WriteToplist( "Intelligence", Toplists["intelligence"] );
			WriteToplist( "Wisdom", Toplists["wisdom"] );
			WriteToplist( "Charisma", Toplists["charisma"] );
		}
		if ( WriteQuery["top-skills"] ) {
			for ( ToplistMap::iterator i = Toplists.begin(); i != Toplists.end(); i++ ) {
				if ( i->first.substr( 0, 6 ) != "Skill:" ) continue;
				WriteToplist( i->first, i->second );
			}
		}
		if ( WriteQuery["top-saves"] ) {
			WriteToplist( "Fort Save", Toplists["save-fort"] );
			WriteToplist( "Refl Save", Toplists["save-refl"] );
			WriteToplist( "Will Save", Toplists["save-will"] );
		}
		if ( WriteQuery["top-experience"] ) WriteToplist( "Experience", Toplists["experience"] );
		if ( WriteQuery["top-wealth"] ) WriteToplist( "Wealth", Toplists["gold"] );
		if ( WriteQuery["top-youngest"] ) WriteToplist( "Youngest", Toplists["age"], true );
		if ( WriteQuery["top-oldest"] ) WriteToplist( "Oldest", Toplists["age"] );
		if ( WriteQuery["top-itemcount"] ) WriteToplist( "Inventory Size", Toplists["itemcount"] );
		if ( WriteQuery["top-filesize"] ) WriteToplist( "File Size", Toplists["filesize"] );
	}
};

#endif