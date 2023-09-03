build_lib(
    LIBNAME MonteCarloSimulator
    SOURCE_FILES
        model/MonteCarloSimulator.cc
    HEADER_FILES
        model/MonteCarloSimulator.h
    LIBRARIES_TO_LINK
        ${libinternet}
        ${libmobility}
        ${libwifi}
        ${libinternet}
        ${libapplications}
)

