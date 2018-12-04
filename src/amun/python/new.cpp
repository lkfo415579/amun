#include <boost/python.hpp>
#include <boost/thread/tss.hpp>
#include <boost/timer/timer.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "common/exception.h"
#include "common/god.h"
#include "common/logging.h"
#include "common/printer.h"
#include "common/search.h"
#include "common/sentence.h"
#include "common/sentences.h"
#include "common/threadpool.h"
#include "common/translation_task.h"

using namespace amunmt;
using namespace std;

God god_;

void init(const std::string &options) { god_.Init(options); }

boost::python::list translate_batch(boost::python::list &in) {
  size_t miniSize = god_.Get<size_t>("mini-batch");
  size_t maxiSize = god_.Get<size_t>("maxi-batch");
  int miniWords = god_.Get<int>("mini-batch-words");

  std::vector<std::future<std::shared_ptr<Histories>>> results;
  SentencesPtr maxiBatch(new Sentences());

  for (int lineNum = 0; lineNum < boost::python::len(in); ++lineNum) {
    std::string line =
        boost::python::extract<std::string>(boost::python::object(in[lineNum]));
    // cerr << "line=" << line << endl;

    maxiBatch->push_back(SentencePtr(new Sentence(god_, lineNum, line)));

    if (maxiBatch->size() >= maxiSize) {

      maxiBatch->SortByLength();
      while (maxiBatch->size()) {
        SentencesPtr miniBatch = maxiBatch->NextMiniBatch(miniSize, miniWords);

        results.emplace_back(god_.GetThreadPool().enqueue(
            [miniBatch] { return TranslationTask(::god_, miniBatch); }));
      }

      maxiBatch.reset(new Sentences());
    }
  }

  // last batch
  if (maxiBatch->size()) {
    maxiBatch->SortByLength();
    while (maxiBatch->size()) {
      SentencesPtr miniBatch = maxiBatch->NextMiniBatch(miniSize, miniWords);
      results.emplace_back(god_.GetThreadPool().enqueue(
          [miniBatch] { return TranslationTask(::god_, miniBatch); }));
    }
  }

  // resort batch into line number order
  Histories allHistories;
  for (auto &&result : results) {
    std::shared_ptr<Histories> histories = result.get();
    allHistories.Append(*histories);
  }
  allHistories.SortByLineNum();

  // output
  std::stringstream ss;
  boost::python::list output;
  std::cerr << "FUCK" << std::endl;
  for (size_t i = 0; i < allHistories.size(); ++i) {
    const History &history = *allHistories.at(i).get();
    const Sentence &sentence = *maxiBatch->at(i).get();
    Printer(god_, history, ss, sentence);
    string str = ss.str();
    std::cerr << str << std::endl;
    output.append(str);
    ss.str("");
  }
  std::cerr << "DAMN" << std::endl;

  return output;
}

BOOST_PYTHON_MODULE(libfastnmt) {
  boost::python::def("init", init);
  boost::python::def("translate_batch", translate_batch);
}
