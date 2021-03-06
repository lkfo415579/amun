#include "translation_task.h"

#include <string>

#ifdef CUDA
#include <thrust/system_error.h>
#endif

#include "search.h"
#include "output_collector.h"
#include "printer.h"
#include "history.h"

using namespace std;

namespace amunmt {

void TranslationTaskAndOutput(const God &god, std::shared_ptr<Sentences> sentences) {
  OutputCollector &outputCollector = god.GetOutputCollector();

  std::shared_ptr<Histories> histories = TranslationTask(god, sentences);

  for (unsigned i = 0; i < histories->size(); ++i) {
    const History &history = *histories->at(i);
    unsigned lineNum = history.GetLineNum();
    const Sentence &sentence = sentences->Get(0);

    std::stringstream strm;
    Printer(god, history, strm, sentence);

    outputCollector.Write(lineNum, strm.str());
  }
}

std::shared_ptr<Histories> TranslationTask(const God &god, std::shared_ptr<Sentences> sentences) {
  try {
    // cerr << "T" << endl;
    Search& search = god.GetSearch();
    // cerr << "T" << endl;
    // cerr << "GOD:" << god.Get_UNK() << endl;
    bool allow_unk = god.Get_UNK();
    auto histories = search.Translate(*sentences, allow_unk);

    return histories;
  }
#ifdef CUDA
  catch(thrust::system_error &e)
  {
    std::cerr << "CUDA error during some_function: " << e.what() << std::endl;
    abort();
  }
#endif
  catch(std::bad_alloc &e)
  {
    std::cerr << "Bad memory allocation during some_function: " << e.what() << std::endl;
    abort();
  }
  catch(std::runtime_error &e)
  {
    std::cerr << "Runtime error during some_function: " << e.what() << std::endl;
    abort();
  }
  catch(...)
  {
    std::cerr << "Some other kind of error during some_function" << std::endl;
    abort();
  }

}

}
