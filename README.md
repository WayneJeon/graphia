# [Graphia](https://graphia.app/)
Graphia is a powerful open source visual analytics application developed to aid the interpretation of large and complex datasets.

https://graphia.app/

[User guide](https://graphia.app/userguide.html)

## **Features**

- Support for a variety of input data formats, ranging from raw CSV to GraphML
- Create correlation graphs using algorithms such as the Pearson Correlation Coefficient
- Visualisation of millions of data points and relationships
- Interactive visualisation and layout in 2D or 3D
- Flexible search facilities, based on attribute information
- Fast, tunable network clustering using Louvain or MCL algorithms
- Graph metric algorithms including PageRank, Betweeness and Eccentricity
- Enrichment Analysis
- Filter graph elements based on numeric or string based attribute expressions
- Customisable and simple to use web search
- Easy export and sharing of analysis results

## **Many Data Types from Many Sectors**

- **Biological Sciences** - protein interaction data, transcriptomics, single cell analyses, proteomics, metabolomics, multiparameter flow cytometry, genotyping data, medical imaging data
- **Agritech** - data relating to the performance of animals, crops, farms, etc.
- **Fintech** - any numerical data relating to changing variables over time, e.g. share prices or categorical data relating to the attributes of commercial entities
- **Social Media** - network connections between individuals, companies, etc.
- **Text Mining** - count matrices of words found across many documents
- **Questionnaire** - answers to questions are categorical (yes/no) or continuous (1-10)

## **About**

Graphia is designed and built by a small dedicated group of scientists working in Edinburgh, Scotland. We are passionate about graphs, the power of visualisation and creating tools that aid the interpretation of complex data. 

Our journey started 20 years ago, as we began to envisage how graphs could help us analyse the relationships between genes and proteins. Over this time we developed multiple tools and refined our approach, eventually arriving where we are today with Graphia. We believe it is the best option for interactively visualising large graphs.

## **Working with us**

There are lots of new features and functionality we still wish to add to Graphia and envisage a desire from some to integrate it into local data infrastructures or web resources. If you are interested in working with us to further improve Graphia or need our help in other ways, we would love to hear from you.

Please contact: <info@graphia.app>

If you use Graphia and would like to support the project, we would very much appreciate and gratefully accept donations.


Please help us to help you!

## **Acknowledgements** ##

We would like to thank those who have helped us develop Graphia:

* [Scottish Enterprise](https://www.scottish-enterprise.com/)
* [The Roslin Institute](https://www.roslin.ed.ac.uk/)
* [The University of Edinburgh](https://www.ed.ac.uk/)
* [BBSRC](https://bbsrc.ukri.org/)

## Building ##

Graphia uses the [CMake](https://cmake.org/) build system. A full build can be performed using the following command:
```
cmake -B build && cmake --build build --parallel
```
Note however that you will usually also need Qt 5 to be installed and indicate to CMake where it lives:
```
CMAKE_PREFIX_PATH=/example/path/to/Qt/5.14.2/gcc_64/ cmake -B build && cmake --build build --parallel
```
