#pragma once

#include <wx/wx.h>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <functional>

// 前向声明，避免头部强依赖
class CanvasElement;
class Wire;

enum class PinType {
    INPUT,
    OUTPUT
};

enum class SimEventType {
    PIN_STATE_CHANGE,
    ELEMENT_EVALUATE
};

struct SimEvent {
    SimEventType type;
    size_t elementIndex;
    size_t pinIndex;
    PinType pinType;
    int newState;

    SimEvent(SimEventType t, size_t elem, size_t pin, PinType ptype, int state)
        : type(t), elementIndex(elem), pinIndex(pin), pinType(ptype), newState(state) {
    }
};

struct PinConnection {
    size_t elementIndex;
    size_t pinIndex;
    PinType pinType;
    wxPoint position;
    std::set<std::pair<size_t, size_t>> connectedWires;

    PinConnection(size_t elem, size_t pin, PinType ptype, const wxPoint& pos)
        : elementIndex(elem), pinIndex(pin), pinType(ptype), position(pos) {
    }
};

struct ElementSimInfo {
    size_t index;
    const CanvasElement* element; // 只读指针
    std::vector<PinConnection> inputPins;
    std::vector<PinConnection> outputPins;
    bool needsEvaluation;
    int outputState; // 0: X (invalid), 1: 0 (low), 2: 1 (high)

    ElementSimInfo(size_t idx, const CanvasElement* elem);
};

struct WireConnection {
    size_t wireIndex;
    const Wire* wire;
    std::pair<size_t, size_t> startConnection;
    std::pair<size_t, size_t> endConnection;
    bool isConnected;

    WireConnection(size_t idx, const Wire* w)
        : wireIndex(idx), wire(w), startConnection({ (size_t)-1,(size_t)-1 }), endConnection({ (size_t)-1,(size_t)-1 }), isConnected(false) {
    }
};

class SimulationEngine {
public:
    using StateChangedCallback = std::function<void(size_t elementIndex, int newState)>;

    SimulationEngine();
    ~SimulationEngine();

    void Initialize(const std::vector<CanvasElement>& elements, const std::vector<Wire>& wires);
    void StartSimulation();
    void StopSimulation();
    void ToggleInputPin(size_t elementIndex);
    int GetPinState(size_t elementIndex, size_t pinIndex, PinType pinType) const;
    std::vector<std::vector<int>> GetAllElementStates() const;
    bool HandleDoubleClick(const wxPoint& clickPos);
    bool IsClickOnInteractivePin(const wxPoint& pos, size_t& elementIndex) const;
    void UpdateElementState(size_t elementIndex);

    // 回调设置
    void SetStateChangedCallback(StateChangedCallback cb);

private:
    void BuildConnectionGraph();
    void ProcessSimulationEvent(const SimEvent& event);
    void EvaluateElement(size_t elementIndex);
    void PropagatePinStateChange(size_t elementIndex, size_t pinIndex, PinType pinType, int newState);
    void PropagateThroughWires(const PinConnection& sourcePin, int newState);
    int EvaluateBasicGate(const ElementSimInfo& elemInfo);
    bool CheckPinConnection(const wxPoint& pinPos, const wxPoint& wirePoint) const;
    PinType GetElementPinType(const CanvasElement& elem, size_t pinIndex) const;

private:
    bool m_isRunning;
    std::vector<ElementSimInfo> m_elements;
    std::vector<WireConnection> m_wires;
    std::queue<SimEvent> m_eventQueue;
    std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> m_connectionGraph;
    StateChangedCallback m_stateChangedCb;
    static const int CLICK_TOLERANCE = 8;
};