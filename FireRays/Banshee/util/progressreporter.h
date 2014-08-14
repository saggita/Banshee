#ifndef PROGRESSREPORTER_H
#define PROGRESSREPORTER_H

///< Progress reporter interface is used by the renderers 
///< to report their progress scaled to [0..1] interval
///<
class ProgressReporter
{
public:
    // Destructor
    virtual ~ProgressReporter(){}
    // Progress callback: pass current progress in percents.
    // The caller decides on the resolution, but reporter 
    // is free to coarsen it
    virtual void Report(float progress) = 0;
};

#endif // PROGRESSREPORTER_H