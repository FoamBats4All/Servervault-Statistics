#include "Precomp.h"
#include "StatisticsWriter.h"

// Data types for surfing cached 2DA data.
typedef std::pair<unsigned long, std::string> StringIndex;
typedef std::vector<StringIndex> Index2DA;

// Get a list of values from a 2DA file, given a column.
Index2DA GetStringArray2DA( ResourceManager &resources, std::string f2da, std::string column ) {
	// Data vector.
	Index2DA Data;

	// For each row in the 2da...
	size_t RowCount = resources.Get2DARowCount( f2da.c_str() );
	for ( size_t Row = 0; Row < RowCount; Row += 1 ) {
		// Get the row field.
		std::string Name;
		if ( !resources.Get2DAString( f2da.c_str(), column.c_str(), Row, Name ) ) continue;

		// Add to the data list.
		Data.push_back( StringIndex( Row, Name ) );
	}
	return Data;
}

// Get the string values of TLK id values from a 2DA file, given a column.
Index2DA GetStringRefArray2DA( ResourceManager &resources, std::string f2da, std::string column ) {
	// Data vector.
	Index2DA Data;

	// For each row in the 2da...
	size_t RowCount = resources.Get2DARowCount( f2da.c_str() );
	for ( size_t r = 0; r < RowCount; r++ ) {
		// Get the row field.
		unsigned long TlkID;
		if ( !resources.Get2DAUlong( f2da.c_str(), column.c_str(), r, TlkID ) ) continue;
		
		// Convert it to a string from the tlk file.
		std::string Name;
		resources.GetTalkString( TlkID, Name );

		// Push to data list.
		Data.push_back( StringIndex( r, Name ) );
	}
	return Data;
}

// Entry point.
int main( int argc, char** argv ) {
	// Allocate the reader.
	PrintfTextOut TextOut;

	// ...
	try {
		// Read the ini file.
		boost::program_options::options_description ini_desc;
		ini_desc.add_options()
			( "settings.module", "Name of the module to load." )
			( "settings.recentonly", "Skips bics older than exclude.days old." )
			( "settings.topcount", "Number of 'top' records to display." )
			( "settings.format", "Format style." )
			( "paths.nwn2-install", "Path that NWN2 is installed in." )
			( "paths.nwn2-home", "Path where NWN2 user files are stored." )
			( "statistics.top", "Display statistics for record holders." )
			( "statistics.gender", "Display statistics on gender." )
			( "statistics.race", "Display statistics on race." )
			( "statistics.subrace", "Display statistics on subrace." )
			( "statistics.background", "Display statistics on backgrounds." )
			( "statistics.alignment", "Display statistics on alignment." )
			( "statistics.deity", "Display deity statistics." )
			( "statistics.levels", "Display class level statistics." )
			( "statistics.skills", "Display skill rank statistics." )
			( "statistics.feats", "Display feat usage statistics." )
			( "statistics.tails", "Display tail model statistics." )
			( "statistics.wings", "Display wing model statistics." )
			( "toplists.health", "Display top x based on HP." )
			( "toplists.armorclass", "Display top x based on AC." )
			( "toplists.baseattackbonus", "Display top x based on BAB." )
			( "toplists.abilities", "Display top x based on abilities." )
			( "toplists.skills", "Display top x based on skills." )
			( "toplists.saves", "Display top x based on saving throws." )
			( "toplists.experience", "Display top x based on experience." )
			( "toplists.wealth", "Display top x based on gold." )
			( "toplists.youngest", "Display youngest x characters." )
			( "toplists.oldest", "Display oldest x characters." )
			( "toplists.itemcount", "Display top x based on inventory size." )
			( "toplists.filesize", "Display top x based on file size." )
			( "exclude.days", "Ignore bics older than this." );
		std::ifstream ini_file( "ServervaultStatistics.ini" );
		boost::program_options::variables_map ini;
		boost::program_options::store( boost::program_options::parse_config_file( ini_file, ini_desc, true ), ini );

		// Get the check module.
		if ( !ini.count( "settings.module" ) ) throw std::exception( "Module not set." );
		std::string ModuleName = ini["settings.module"].as<std::string>();

		// Get the NWN2 install location.
		if ( !ini.count( "paths.nwn2-install" ) ) throw std::exception( "NWN2 install location not set." );
		std::string PathNWN2Install = ini["paths.nwn2-install"].as<std::string>();
		if ( !boost::filesystem::exists( PathNWN2Install ) ) throw std::exception( "NWN2 install location does not exist." );

		// Get the NWN2 home location.
		if ( !ini.count( "paths.nwn2-home" ) ) throw std::exception( "NWN2 home location not set." );
		std::string PathNWN2Home = ini["paths.nwn2-home"].as<std::string>();
		if ( !boost::filesystem::exists( PathNWN2Home ) ) throw std::exception( "NWN2 home location does not exist." );

		// Get the servervault location..
		boost::filesystem::path servervault( PathNWN2Home + "\\servervault" );
		if ( !boost::filesystem::exists(servervault) || !boost::filesystem::is_directory(servervault) ) throw std::exception( "Servervault folder could not be found." );

		// Get cutoff data.
		time_t now = time( NULL );
		double cutofftime = 0;
		if ( ini["settings.recentonly"].as<std::string>() == "1" ) {
			cutofftime = boost::lexical_cast<double>( ini["exclude.days"].as<std::string>() ) * 60 * 60 * 24;
		}

		// Load the NWN2 Resource Manager and module.
		TextOut.WriteText( "Loading resources and module ..." );
		ResourceManager resources( &TextOut );
		LoadModule( resources, ModuleName.c_str(), PathNWN2Home.c_str(), PathNWN2Install.c_str() );
		
		// Prepare statistics writer.
		TextOut.WriteText( "\nPreparing writer ..." );
		StatisticsWriter writer( "ServervaultStatistics.log" );
		writer.Format = boost::lexical_cast<int>( ini["settings.format"].as<std::string>().c_str() );
		writer.ToplistMax = boost::lexical_cast<unsigned int>( ini["settings.topcount"].as<std::string>().c_str() );
		writer.CountedBics = 0;
		writer.IgnoredBics = 0;

		// Easy checks.
		std::map<std::string,bool> showSettings;
		showSettings["gender"] = ( ini["statistics.gender"].as<std::string>() == "1" );
		showSettings["race"] = ( ini["statistics.race"].as<std::string>() == "1" );
		showSettings["subrace"] = ( ini["statistics.subrace"].as<std::string>() == "1" );
		showSettings["background"] = ( ini["statistics.background"].as<std::string>() == "1" );
		showSettings["alignment"] = ( ini["statistics.alignment"].as<std::string>() == "1" );
		showSettings["deity"] = ( ini["statistics.deity"].as<std::string>() == "1" );
		showSettings["levels"] = ( ini["statistics.levels"].as<std::string>() == "1" );
		showSettings["skills"] = ( ini["statistics.skills"].as<std::string>() == "1" );
		showSettings["feats"] = ( ini["statistics.feats"].as<std::string>() == "1" );
		showSettings["tails"] = ( ini["statistics.tails"].as<std::string>() == "1" );
		showSettings["wings"] = ( ini["statistics.wings"].as<std::string>() == "1" );
		showSettings["top"] = ( ini["statistics.top"].as<std::string>() == "1" );
		if ( showSettings["top"] ) {
			showSettings["top-health"] = ( ini["toplists.health"].as<std::string>() == "1" );
			showSettings["top-armorclass"] = ( ini["toplists.armorclass"].as<std::string>() == "1" );
			showSettings["top-baseattackbonus"] = ( ini["toplists.baseattackbonus"].as<std::string>() == "1" );
			showSettings["top-abilities"] = ( ini["toplists.abilities"].as<std::string>() == "1" );
			showSettings["top-skills"] = ( ini["toplists.skills"].as<std::string>() == "1" );
			showSettings["top-saves"] = ( ini["toplists.saves"].as<std::string>() == "1" );
			showSettings["top-experience"] = ( ini["toplists.experience"].as<std::string>() == "1" );
			showSettings["top-wealth"] = ( ini["toplists.wealth"].as<std::string>() == "1" );
			showSettings["top-youngest"] = ( ini["toplists.youngest"].as<std::string>() == "1" );
			showSettings["top-oldest"] = ( ini["toplists.oldest"].as<std::string>() == "1" );
			showSettings["top-itemcount"] = ( ini["toplists.itemcount"].as<std::string>() == "1" );
			showSettings["top-filesize"] = ( ini["toplists.filesize"].as<std::string>() == "1" );
		} else {
			showSettings["top-health"] = false;
			showSettings["top-armorclass"] = false;
			showSettings["top-baseattackbonus"] = false;
			showSettings["top-abilities"] = false;
			showSettings["top-skills"] = false;
			showSettings["top-saves"] = false;
			showSettings["top-experience"] = false;
			showSettings["top-wealth"] = false;
			showSettings["top-youngest"] = false;
			showSettings["top-oldest"] = false;
			showSettings["top-itemcount"] = false;
			showSettings["top-filesize"] = false;
		}
		
		// Format statistics query.
		writer.WriteQuery["top"] = showSettings["top"];
		writer.WriteQuery["gender"] = showSettings["gender"];
		writer.WriteQuery["race"] = showSettings["race"];
		writer.WriteQuery["subrace"] = showSettings["subrace"];
		writer.WriteQuery["background"] = showSettings["background"];
		writer.WriteQuery["alignment"] = showSettings["alignment"];
		writer.WriteQuery["deity"] = showSettings["deity"];
		writer.WriteQuery["levels"] = showSettings["levels"];
		writer.WriteQuery["skills"] = showSettings["skills"];
		writer.WriteQuery["feats"] = showSettings["feats"];
		writer.WriteQuery["tails"] = showSettings["tails"];
		writer.WriteQuery["wings"] = showSettings["wings"];

		// Format toplist query.
		if ( writer.ToplistMax > 0 && showSettings["top"] ) {
			writer.WriteQuery["top-health"] = showSettings["top-health"];
			writer.WriteQuery["top-armorclass"] = showSettings["top-armorclass"];
			writer.WriteQuery["top-baseattackbonus"] = showSettings["top-baseattackbonus"];
			writer.WriteQuery["top-abilities"] = showSettings["top-abilities"];
			writer.WriteQuery["top-skills"] = showSettings["top-skills"];
			writer.WriteQuery["top-saves"] = showSettings["top-saves"];
			writer.WriteQuery["top-experience"] = showSettings["top-experience"];
			writer.WriteQuery["top-wealth"] = showSettings["top-wealth"];
			writer.WriteQuery["top-youngest"] = showSettings["top-youngest"];
			writer.WriteQuery["top-oldest"] = showSettings["top-oldest"];
			writer.WriteQuery["top-itemcount"] = showSettings["top-itemcount"];
			writer.WriteQuery["top-filesize"] = showSettings["top-filesize"];
		}

		// Statistics/toplist containers.
		StatisticMap Statistics;
		ToplistMap Toplists;

		// Prepare 2da data - Genders.
		TextOut.WriteText( "\nIndexing 2da files ..." );
		Index2DA Genders = GetStringArray2DA( resources, "gender", "GENDER" );
		for ( Index2DA::iterator i = Genders.begin(); i < Genders.end(); i++ ) Statistics["gender"][i->second] = 0;
		
		// Prepare 2da data - Races.
		Index2DA Races = GetStringRefArray2DA( resources, "racialtypes", "Name" );
		for ( Index2DA::iterator i = Races.begin(); i < Races.end(); i++ ) Statistics["race"][i->second] = 0;
		
		// Prepare 2da data - Subraces.
		Index2DA Subraces = GetStringRefArray2DA( resources, "racialsubtypes", "Name" );
		for ( Index2DA::iterator i = Subraces.begin(); i < Subraces.end(); i++ ) Statistics["subrace"][i->second] = 0;
		
		// Prepare 2da data - Classes.
		Index2DA Classes = GetStringRefArray2DA( resources, "classes", "Name" );
		for ( Index2DA::iterator i = Classes.begin(); i < Classes.end(); i++ ) Statistics["levels"][i->second] = 0;
		
		// Prepare 2da data - Skills.
		Index2DA Skills = GetStringRefArray2DA( resources, "skills", "Name" );
		for ( Index2DA::iterator i = Skills.begin(); i < Skills.end(); i++ ) Statistics["skills"][i->second] = 0;
		
		// Prepare 2da data - Skills.
		Index2DA Feats = GetStringRefArray2DA( resources, "feat", "FEAT" );
		for ( Index2DA::iterator i = Feats.begin(); i < Feats.end(); i++ ) Statistics["feats"][i->second] = 0;
		
		// Prepare 2da data - Backgrounds.
		Index2DA Backgrounds = GetStringRefArray2DA( resources, "backgrounds", "Name" );
		for ( Index2DA::iterator i = Backgrounds.begin(); i < Backgrounds.end(); i++ ) Statistics["background"][i->second] = 0;
		
		// Prepare 2da data - Tails.
		Index2DA Tails = GetStringRefArray2DA( resources, "tailmodel", "StringRef" );
		for ( Index2DA::iterator i = Tails.begin(); i < Tails.end(); i++ ) Statistics["tails"][i->second] = 0;
		
		// Prepare 2da data - Wings.
		Index2DA Wings = GetStringRefArray2DA( resources, "wingmodel", "StringRef" );
		for ( Index2DA::iterator i = Wings.begin(); i < Wings.end(); i++ ) Statistics["wings"][i->second] = 0;
		
		// Get the bic file data.
		TextOut.WriteText( "\nGathering character data ..." );
		typedef std::vector<boost::filesystem::path> PathVec;
		PathVec players;
		std::copy( boost::filesystem::directory_iterator(servervault), boost::filesystem::directory_iterator(), std::back_inserter(players) );
		for ( PathVec::iterator p = players.begin(); p < players.end(); p++ ) {
			PathVec characters;
			std::copy( boost::filesystem::directory_iterator(*p), boost::filesystem::directory_iterator(), std::back_inserter(characters) );
			for ( PathVec::iterator c = characters.begin(); c < characters.end(); c++ ) {
				// Check for 0-byte characters.
				uintmax_t filesize = boost::filesystem::file_size( *c );
				if ( filesize == 0 ) {
					// Compile string.
					std::stringstream ss;
					ss << "Zero-size file: ";
					ss << *c;

					// Log the warning.
					writer.LogWarning( ss.str() );

					// Skip file.
					continue;
				}

				// Gather the data.
				try {
					// Load b data.
					if ( c->extension() != ".bic" ) continue;
					CharacterBic* b = new CharacterBic( resources, c->string() );
					b->player = c->parent_path().filename().string();
					b->filesize = boost::filesystem::file_size( *c );
					b->lastmodified = boost::filesystem::last_write_time( *c );
				
					// Ignore if it's past the cutoff date.
					if ( cutofftime != 0 ) {
						double timedif = difftime( now, b->lastmodified );
						if ( timedif > cutofftime ) {
							writer.IgnoredBics++;
							delete b;
							continue;
						}
					}
				
					// Update easy statistics.
					std::string Name = b->GetFullName();
					if ( showSettings["gender"] ) Statistics["gender"][b->GetGffToString( "Gender", "gender", "GENDER" )]++;
					if ( showSettings["race"] ) Statistics["race"][b->GetGffToTlkString( "Race", "racialtypes", "Name" )]++;
					if ( showSettings["subrace"] ) Statistics["subrace"][b->GetGffToTlkString( "Subrace", "racialsubtypes", "Name" )]++;
					if ( showSettings["background"] ) Statistics["background"][b->GetGffToTlkString( "CharBackground", "backgrounds", "Name" )]++;
					if ( showSettings["deity"] ) Statistics["deity"][b->GetString( "Deity" )]++;
					if ( showSettings["alignment"] ) Statistics["alignment"][b->GetAlignment()]++;
					if ( showSettings["tails"] ) Statistics["tails"][b->GetGffToTlkString( "Tail", "tailmodel", "StringRef" )]++;
					if ( showSettings["wings"] ) Statistics["wings"][b->GetGffToTlkString( "Wings", "wingmodel", "StringRef" )]++;

					// Update class levels.
					if ( showSettings["levels"] ) {
						for ( Index2DA::iterator c = Classes.begin(); c < Classes.end(); c++ )
							Statistics["levels"][c->second] += b->GetClassLevels( c->first );
					}

					// Update skill data.
					if ( showSettings["skills"] || showSettings["top-skills"] ) {
						for ( Index2DA::iterator s = Skills.begin(); s < Skills.end(); s++ ) {
							int ranks = b->GetSkillRanks( s->first );
							Statistics["skills"][s->second] += ranks;
							Toplists["Skill: " + s->second].push_back( ToplistPair( ranks, Name ) );
						}
					}

					// Update feat data.
					if ( showSettings["feats"] ) {
						for ( Index2DA::iterator s = Feats.begin(); s < Feats.end(); s++ ) {
							if ( b->GetHasFeat( s->first ) ) Statistics["feats"][s->second]++;
						}
					}

					// Update toplists.
					if ( showSettings["top-health"] ) Toplists["health"].push_back( ToplistPair( b->GetIntUnsigned( "HitPoints" ), Name ) );
					if ( showSettings["top-armorclass"] ) Toplists["armorclass"].push_back( ToplistPair( b->GetIntUnsigned( "ArmorClass" ), Name ) );
					if ( showSettings["top-baseattackbonus"] ) Toplists["baseattackbonus"].push_back( ToplistPair( b->GetIntUnsigned( "BaseAttackBonus" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["strength"].push_back( ToplistPair( b->GetIntUnsigned( "Str" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["dexterity"].push_back( ToplistPair( b->GetIntUnsigned( "Dex" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["constitution"].push_back( ToplistPair( b->GetIntUnsigned( "Con" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["intelligence"].push_back( ToplistPair( b->GetIntUnsigned( "Int" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["wisdom"].push_back( ToplistPair( b->GetIntUnsigned( "Wis" ), Name ) );
					if ( showSettings["top-abilities"] ) Toplists["charisma"].push_back( ToplistPair( b->GetIntUnsigned( "Cha" ), Name ) );
					if ( showSettings["top-saves"] ) Toplists["save-fort"].push_back( ToplistPair( b->GetInt( "FortSaveThrow" ), Name ) );
					if ( showSettings["top-saves"] ) Toplists["save-refl"].push_back( ToplistPair( b->GetInt( "RefSaveThrow" ), Name ) );
					if ( showSettings["top-saves"] ) Toplists["save-will"].push_back( ToplistPair( b->GetInt( "WillSaveThrow" ), Name ) );
					if ( showSettings["top-wealth"] ) Toplists["gold"].push_back( ToplistPair( b->GetIntUnsigned( "Gold" ), Name ) );
					if ( showSettings["top-experience"] ) Toplists["experience"].push_back( ToplistPair( b->GetIntUnsigned( "Experience" ), Name ) );
					if ( showSettings["top-youngest"] || showSettings["top-oldest"] ) Toplists["age"].push_back( ToplistPair( b->GetIntUnsigned( "Age" ), Name ) );
					if ( showSettings["top-itemcount"] ) Toplists["itemcount"].push_back( ToplistPair( b->GetInventorySize(), Name ) );
					if ( showSettings["top-filesize"] ) Toplists["filesize"].push_back( ToplistPair( b->filesize, Name ) );

					delete b;
					writer.CountedBics++;
				} catch ( std::exception &e ) {
					// Compile string.
					std::stringstream ss;
					ss << *c;
					ss << " : ";
					ss << e.what();

					// Log the warning.
					writer.LogWarning( ss.str() );
				}
			}

			// Cut off the toplists after each player.
			for ( ToplistMap::iterator i = Toplists.begin(); i != Toplists.end(); i++ ) {
				if ( i->second.size() < ( writer.ToplistMax * 2 + 2 ) ) continue;
				std::sort( i->second.rbegin(), i->second.rend() );
				i->second.erase( i->second.begin() + writer.ToplistMax + 1, i->second.end() - writer.ToplistMax - 1 );
			}
		}

		// Output data.
		TextOut.WriteText( "\nWriting statistics ..." );
		writer.WriteStatistics( Statistics );
		writer.WriteToplists( Toplists );
	} catch ( std::exception &e ) {
		TextOut.WriteText( "\nError: %s\n", e.what() );
		system( "PAUSE" );
		return EXIT_FAILURE;
	}

	// Exit
	return EXIT_SUCCESS;
}