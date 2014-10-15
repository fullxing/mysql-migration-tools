#include "replication_patterns.h"
#include "../lib_common/file.h"
#include "../lib_common/directory.h"
#include "../lib_common/ini_file.h"
#include "../lib_tinyxml/tinyxml.h"  
#include "../lib_tinyxml/tinystr.h" 

#include <sstream>
#include <assert.h>

using namespace std;

ReplicationState* ReplicationState::instance = NULL;

ReplicationState& ReplicationState::getInstance()
{
    if (instance == NULL) {
        instance = new ReplicationState();
    } 
    
    return (*instance);
}

bool ReplicationState::init_relication_info() {
    string currentPath = "";
    Directory::getCurrentPath(currentPath); 
    
    string patterns_abs_path = currentPath + patterns_file;
    if (false == File::exist(patterns_abs_path)) {
        printf("patterns file not exist. file path:%s.\n", patterns_abs_path.c_str()); 
        return false;
    }
    
    TiXmlDocument doc_patterns(patterns_abs_path.c_str());  
    if(false == doc_patterns.LoadFile())
    {
        printf("load patterns file failure. file:%s, error info:%s.\n", 
                patterns_abs_path.c_str(),
                doc_patterns.ErrorDesc()); 
        return false;
    }
    
    TiXmlElement *root_element = doc_patterns.RootElement();  
    if(root_element == NULL)
    {
        printf("get root element failure. \n"); 
        return false;
    }
    
    TiXmlElement *sources_element = root_element->FirstChildElement("Sources");
    if(sources_element == NULL)
    {
        printf("get sources element failure.\n"); 
        return false;
    }
    
    TiXmlElement *source_element = sources_element->FirstChildElement("Source");
    while(NULL != source_element)
    {             
        TiXmlElement *ip_element   = source_element->FirstChildElement("IP");
        TiXmlElement *port_element = source_element->FirstChildElement("Port");
        
        MasterInfo master;  
        master.ip   = ip_element->FirstChild()->Value();
        master.port = atoi(port_element->FirstChild()->Value());
        
        TiXmlElement *file_element     = source_element->FirstChildElement("File");
        TiXmlElement *position_element = source_element->FirstChildElement("Position");
        
        ReplicationInfo replication;
        replication.bin_log_file = file_element->FirstChild()->Value();
        replication.position     = atol(position_element->FirstChild()->Value());
            
        // add master node info
        add_master_node(master, replication);
       
        source_element = source_element->NextSiblingElement("Source");
    }
    
    return true;
}

bool ReplicationState::update_replication_state(MasterInfo& master, ReplicationInfo& replication)
{
    std::map<MasterInfo, ReplicationInfo>::iterator iterator_replications;
    iterator_replications = replications.find(master);
    if(iterator_replications == replications.end())
        return false;

    replications[master] = replication;
    return true;
}

bool ReplicationState::save_replication_info()
{
    string currentPath = "";
    Directory::getCurrentPath(currentPath); 
    
    string replication_state_abs_path = currentPath + replication_state_file;
           
    IniFile inifile(replication_state_abs_path);
    if(0 == inifile.write_profile_string("ErrorInfo", "Description", error_description)){
        printf("save replication info error desc failure.\n");
        return false;
    }
    
    if(0 == inifile.write_profile_string("ErrorInfo", "IP", error_ip)){
        printf("save replication info error IP failure.\n");
        return false;
    }
    
    if(0 == inifile.write_profile_string("ErrorInfo", "Port", error_port)){
        printf("save replication info error port failure.\n");
        return false;
    }
    
    if(0 == inifile.write_profile_string("ErrorInfo", "BinLogFile", error_bin_log_file)){
        printf("save replication info error bin log file failure.\n");
        return false;
    }
    
    stringstream ss_error;
    ss_error<<error_position;
    if(0 == inifile.write_profile_string("ErrorInfo", "Position", ss_error.str())){
        printf("save replication info error position failure.\n");
        return false;
    }
    
    if(0 == inifile.write_profile_string("ErrorInfo", "Description", error_description)){
        printf("save replication info error desc failure.\n");
        return false;
    }
    
    std::map<MasterInfo, ReplicationInfo>::iterator iterator_replications;
    for(iterator_replications = replications.begin(); iterator_replications != replications.end(); ++iterator_replications)
    {
        string section = "Replication_" + get_master_desc(iterator_replications->first);
        if(0 == inifile.write_profile_string(section, "BinLogFile", iterator_replications->second.bin_log_file)){
            printf("save replication info node file failure.\n");
            return false;
        }
        
        stringstream ss_node_pos;
        ss_node_pos<<iterator_replications->second.position;
        if(0 == inifile.write_profile_string(section, "Position", ss_node_pos.str())){
            printf("save replication info node position failure.\n");
            return false;
        }
    }
    
    return true;
}

bool ReplicationState::save_replication_info(MasterInfo& master, ReplicationInfo& replication)
{
    if(false == update_replication_state(master, replication))
        return false;
    
    string currentPath = "";
    Directory::getCurrentPath(currentPath); 
    
    string replication_state_abs_path = currentPath + replication_state_file;
           
    IniFile inifile(replication_state_abs_path);
    if(0 == inifile.write_profile_string("ErrorInfo", "Description", error_description))
    {
        printf("save replication info error desc failure.\n");
        return false;
    }
    
    string section = "Replication_" + get_master_desc(master);
    if(0 == inifile.write_profile_string(section, "BinLogFile", replication.bin_log_file))
    {
         printf("save replication info node file failure.\n");
         return false;
    }
        
    stringstream ss_node_pos;
    ss_node_pos<<replication.position;
    if(0 == inifile.write_profile_string(section, "Position", ss_node_pos.str()))
    {
         printf("save replication info node position failure.\n");
         return false;
    }
    
    return true;
}

void ReplicationState::add_master_node(const MasterInfo& master, const ReplicationInfo& replication)
{
     replications.insert(std::pair<MasterInfo, ReplicationInfo>(master, replication));
}

string ReplicationState::get_master_desc(const MasterInfo& master) const
{
    string node_desc = "";
    
    stringstream ss;
    ss<<master.port;
    node_desc = master.ip + "_" + ss.str();
    
    return node_desc;
}

ReplicationPatterns* ReplicationPatterns::instance = NULL;

ReplicationPatterns& ReplicationPatterns::getInstance()
{
    if (instance == NULL) {
        instance = new ReplicationPatterns();
    } 
    
    return (*instance);
}

bool ReplicationPatterns::load() 
{
    string currentPath = "";
    Directory::getCurrentPath(currentPath); 
    
    string patterns_abs_path = currentPath + patterns_file;
    if (false == File::exist(patterns_abs_path)) {
        printf("load replication patterns, patterns file not exist. file path:%s.\n", patterns_abs_path.c_str()); 
        return false;
    }
    
    TiXmlDocument doc_patterns(patterns_abs_path.c_str());  
    if(false == doc_patterns.LoadFile())
    {
        printf("load replication patterns, load file failure. file:%s, error info:%s.\n", 
                patterns_abs_path.c_str(),
                doc_patterns.ErrorDesc()); 
        return false;
    }
    
    TiXmlElement *root_element = doc_patterns.RootElement();  
    if(root_element == NULL)
    {
        printf("get root element failure. \n"); 
        return false;
    }
    
    TiXmlElement *mode_element = root_element->FirstChildElement("Mode");
    if(false == load_modes(mode_element))
    {
        printf("load mode element failure.\n"); 
        return false;
    }
    
    TiXmlElement *sources_element = root_element->FirstChildElement("Sources");
    if(false == load_sources(sources_element))
    {
        printf("load sources element failure.\n"); 
        return false;
    }
    
    TiXmlElement *destinations_element = root_element->FirstChildElement("Destinations");
    if(false == load_destinations(destinations_element))
    {
        printf("load destinations element failure.\n"); 
        return false;
    }
     
  
    return true;
}

bool ReplicationPatterns::load_modes(TiXmlElement *mode_node)
{
    if(NULL == mode_node)
    {
        printf("mode element is NULL.\n"); 
        return false;
    }
    
    string mode_type = mode_node->Attribute("type");  
    printf("mode type = %s.\n", mode_type.c_str());
    
    mode = atoi(mode_type.c_str());
       
    return true;
}

bool ReplicationPatterns::load_sources(TiXmlElement *sources_node)
{
    if(NULL == sources_node)
    {
        printf("sources element is NULL.\n"); 
        return false;
    }
    
    TiXmlElement *source_element = sources_node->FirstChildElement("Source");
    while(NULL != source_element)
    {       
        SourceNode source_node;
        TiXmlElement *ip_element   = source_element->FirstChildElement("IP");
        TiXmlElement *port_element = source_element->FirstChildElement("Port");
        
        MasterInfo master;  
        master.ip   = ip_element->FirstChild()->Value();
        master.port = atoi(port_element->FirstChild()->Value());
        source_node.ip   = master.ip;
        source_node.port = master.port;
        
        TiXmlElement *file_element     = source_element->FirstChildElement("File");
        TiXmlElement *position_element = source_element->FirstChildElement("Position");
        
        ReplicationInfo replication;
        replication.bin_log_file = file_element->FirstChild()->Value();
        replication.position     = atol(position_element->FirstChild()->Value());
        source_node.bin_log_file = replication.bin_log_file;
        source_node.position     = replication.position;
        
        TiXmlElement *user_element     = source_element->FirstChildElement("User");
        TiXmlElement *password_element = source_element->FirstChildElement("Password");
        source_node.user     = user_element->FirstChild()->Value();
        source_node.password = password_element->FirstChild()->Value();
        
        //Replicate_do_db
        TiXmlElement *do_db_element = source_element->FirstChildElement("Replicate_do_db");
        if(NULL != do_db_element)
        {
            TiXmlElement *database_element = do_db_element->FirstChildElement("Database");
            while(NULL != database_element)
            {
                string database_name = database_element->FirstChild()->Value();
                source_node.replicate_do_db.push_back(database_name);
                
                database_element = do_db_element->NextSiblingElement("Database");
            }            
        }
        
        // Replicate_ignore_db
        TiXmlElement *ignore_db_element = source_element->FirstChildElement("Replicate_ignore_db");
        if(NULL != do_db_element)
        {
            TiXmlElement *database_element = ignore_db_element->FirstChildElement("Database");
            while(NULL != database_element)
            {
                string database_name = database_element->FirstChild()->Value();
                source_node.replicate_do_db.push_back(database_name);
                
                database_element = ignore_db_element->NextSiblingElement("Database");
            }            
        }
            
        add_master_node(master, replication);
        source_nodes.push_back(source_node);        
        
        source_element = source_element->NextSiblingElement("Source");
    }
    
    return true;
}

bool ReplicationPatterns::load_destinations(TiXmlElement *destinations_node)
{
    if(NULL == destinations_node)
    {
        printf("destinations element is NULL.\n"); 
        return false;
    }
    
    TiXmlElement *destination_element = destinations_node->FirstChildElement("Destination");
    while(NULL != destination_element)
    {       
        DestinationNode dest_node;
        
        TiXmlElement *ip_element       = destination_element->FirstChildElement("IP");
        TiXmlElement *port_element     = destination_element->FirstChildElement("Port");
        TiXmlElement *user_element     = destination_element->FirstChildElement("User");
        TiXmlElement *password_element = destination_element->FirstChildElement("Password");
        TiXmlElement *database_element = destination_element->FirstChildElement("Database");
        
        dest_node.ip       = ip_element->FirstChild()->Value();
        dest_node.port     = atoi(port_element->FirstChild()->Value());
        dest_node.user     = user_element->FirstChild()->Value();
        dest_node.password = password_element->FirstChild()->Value();
        dest_node.database = database_element->FirstChild()->Value();
        
        destination_nodes.push_back(dest_node);        
        
        destination_element = destination_element->NextSiblingElement("Destination");
    }
    
    return true;
}

void ReplicationPatterns::add_master_node(const MasterInfo& master, const ReplicationInfo& replication)
{
     replications.insert(std::pair<MasterInfo, ReplicationInfo>(master, replication));
}

SourceNode& ReplicationPatterns::get_source_node()
{
    if(source_nodes.empty() == true)
    {
        assert(!"source node is not exist!");
    }
    
    return source_nodes[0];
}

string ReplicationPatterns::get_command_line(SourceNode& source)
{
    stringstream source_driver;
    source_driver<<"mysql://"<<source.user<<":"<<source.password<<"@"<<source.ip<<":"<<source.port;
    
    return source_driver.str();
}






SysConfig* SysConfig::m_instance = NULL;

SysConfig& SysConfig::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new SysConfig();
    } 
    
    return (*m_instance);
}

bool SysConfig::load() {
    string currentPath = "";
    Directory::getCurrentPath(currentPath);  
    
    string cfgFile = currentPath + m_cfgFile;
    if (false == File::exist(cfgFile)) {
        printf("config file not exist.\n"); 
        return false;
    }
           
    IniFile inifile(cfgFile);
    if(!inifile.read_profile_string("Source", "File", m_binLogFile, "")){
        printf("get binlog file name failure.\n");
        return false;
    }
    
    m_position = inifile.read_profile_int("Source", "Position", 0);
    if(m_position == 0){
        printf("get binlog position failure.\n");
        return false;
    }
    
    if(!inifile.read_profile_string("Source", "Database", m_sourceDatabase, "")){
        printf("get database name failure.\n");
        return false;
    }
    
    if(!inifile.read_profile_string("Destination", "IP", m_destIP, "")){
        printf("get destination IP failure.\n");
        return false;
    }
    
    m_destPost = inifile.read_profile_int("Destination", "Port", 0);
    if(m_destPost == 0){
        printf("get destionation port failure.\n");
        return false;
    }
    
    if(!inifile.read_profile_string("Destination", "User", m_userName, "")){
        printf("get destination user name failure.\n");
        return false;
    }
    
    if(!inifile.read_profile_string("Destination", "Password", m_userPassword, "")){
        printf("get destination user password failure.\n");
        return false;
    }
    
   if(!inifile.read_profile_string("Destination", "Database", m_destDatabase, "")){
        printf("get destination database password failure.\n");
        return false;
    }
    
    return true;
}