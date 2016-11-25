#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "../http_frame/http_server.hpp"
#include "looka_searchd.hpp"

#include "../looka_log.hpp"
#include "../looka_string_utils.hpp"
#include "../looka_mysql_wrapper.hpp"
#include "../looka_config_parser.hpp"
#include "../looka_config_source.hpp"
#include "../looka_config_index.hpp"


#define DEFAULT_CONFIG_FILENAME "looka.cfg"

void signal_term_handler(int signo) {
  exit(0);
}
void signal_int_handler(int signo) {
  exit(0);
}

void usage(const char* bin_name)
{
  printf("Usage:\n");
  printf("        %s [options]\n\n", bin_name);
  printf("Options:\n");
  printf("        -h:             Show help messages.\n");
  printf("        -f file:        The configuration file.(default is \"%s\")\n", DEFAULT_CONFIG_FILENAME);
}

int main(int argc, char** argv)
{
  const char *config_filename = DEFAULT_CONFIG_FILENAME;
  char opt_char;
  while ((opt_char = getopt(argc, argv, "f:h")) != -1) {
    switch (opt_char) {
    case 'f':
      config_filename = optarg;
      break;
    case 'h':
      usage(argv[0]);
      return EXIT_SUCCESS;
    default:
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  LookaConfigParser parser;
  if (parser.LoadConfig(config_filename))
    return EXIT_FAILURE;

  std::vector<std::string> source_names;
  std::vector<std::string> index_names;
  std::map<std::string, LookaConfigSource*> lc_source;
  std::map<std::string, LookaConfigIndex*>  lc_index;
  typedef std::map<std::string, LookaConfigSource*>::iterator lc_source_iterator_t;
  typedef std::map<std::string, LookaConfigIndex*>::iterator  lc_index_iterator_t;

  source_names = parser.GetSectionNames(LookaConfigSource::mSectionTag);
  index_names  = parser.GetSectionNames(LookaConfigIndex::mSectionTag);
  for (unsigned int i = 0; i < source_names.size(); i++) {
    LookaConfigSource* source_cfg = new LookaConfigSource(&parser, source_names[i]);
    lc_source.insert(make_pair(source_names[i], source_cfg));
  }
  for (unsigned int i = 0; i < index_names.size(); i++) {
    LookaConfigIndex* index_cfg = new LookaConfigIndex(&parser, index_names[i]);
    lc_index.insert(make_pair(index_names[i], index_cfg));
  }
  
  LookaConfigSearchd* lc_searchd = new LookaConfigSearchd(&parser);
  LookaSearchd* searchd = NULL;

  for (unsigned int i = 0; i < index_names.size(); i++) {
    LookaConfigIndex* index_cfg = lc_index[index_names[i]];
    lc_source_iterator_t it = lc_source.find(index_cfg->source);
    if (it == lc_source.end()) {
      _WARNING("[No such source:%s for index:%s, skipping.]",
        index_cfg->source.c_str(), index_names[i].c_str());
      continue;
    }

    LookaConfigSource* source_cfg = it->second;

    searchd = new LookaSearchd(source_cfg, index_cfg, lc_searchd);
    searchd->Init();
    _INFO("searchd init ok");
    break;
  }
	
  close(STDIN_FILENO);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, &signal_term_handler);
	signal(SIGINT, &signal_int_handler);
  
  // Run server in background thread.
  int thread_num  = searchd->m_searchd_cfg->thread_num;
  int listen_port = searchd->m_searchd_cfg->listen_port;

  HttpServer s(listen_port, thread_num);
  s.SetServerName("looka-searchd");
  // Set server handler for process request
  s.SetServerHandler(searchd);
  s.Start();

  pause();
  s.Stop();

  for (lc_source_iterator_t it = lc_source.begin(); it != lc_source.end(); ++it)
    delete it->second;
  for (lc_index_iterator_t it = lc_index.begin(); it != lc_index.end(); ++it )
    delete it->second;

  return EXIT_SUCCESS;
}
