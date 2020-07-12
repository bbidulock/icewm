#ifndef YTRACE_H
#define YTRACE_H

class YTrace {
public:
    YTrace(const char* kind, const char* inst = nullptr, bool busy = true) :
        kind(kind), inst(inst), busy(busy),
        have(kind && traces(kind))
    {
        if (busy && inst && have) {
            show();
        }
    }
    virtual ~YTrace() { }

    void done() {
        if (busy && inst && have) {
            busy = false;
            show();
        }
    }

    void init(const char* inst, bool busy = true) {
        done();
        this->inst = inst;
        this->busy = busy;
        if (busy && inst && have) {
            show();
        }
    }

    static void tracing(const char* conf) {
        YTrace::conf = conf;
    }
    static const char* tracingConf() {
        return conf;
    }

    bool tracing() const {
        return have;
    }

    static bool traces(const char* kind) {
        return conf && strstr(conf, kind);
    }

protected:
    virtual void show() { show(busy, kind, inst); }
    virtual void show(bool busy, const char* kind, const char* inst);
    const char* getKind() const { return kind; }
    const char* getInst() const { return inst; }

private:
    const char* kind;
    const char* inst;
    bool busy, have;
    static const char* conf;
};

class YTraceIcon : public YTrace {
public:
    YTraceIcon(const char* inst = nullptr, bool busy = true) :
        YTrace("icon", inst, busy) { }
    ~YTraceIcon() { }
};

class YTraceConfig : public YTrace {
public:
    YTraceConfig(const char* inst = nullptr, bool busy = true) :
        YTrace("conf", inst, busy) { }
    ~YTraceConfig() { done(); }
};

class YTraceProg : public YTrace {
public:
    YTraceProg(const char* inst = nullptr, bool busy = true) :
        YTrace("prog", inst, busy) { }
    ~YTraceProg() { }
};

#endif
