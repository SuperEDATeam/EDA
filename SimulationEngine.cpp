#include "SimulationEngine.h"
#include "CanvasElement.h"
#include "Wire.h"
#include <algorithm>
#include <cmath>
#include <iostream>

SimulationEngine::SimulationEngine() : m_isRunning(false) {
}

SimulationEngine::~SimulationEngine() {
    StopSimulation();
}

ElementSimInfo::ElementSimInfo(size_t idx, const CanvasElement* elem)
    : index(idx), element(elem), needsEvaluation(false), outputState(0) {

    const auto& inputs = elem->GetInputPins();
    for (size_t i = 0; i < inputs.size(); ++i) {
        wxPoint pinPos = elem->GetPos() + wxPoint(inputs[i].pos.x, inputs[i].pos.y);
        inputPins.emplace_back(idx, i, PinType::INPUT, pinPos);
    }

    const auto& outputs = elem->GetOutputPins();
    for (size_t i = 0; i < outputs.size(); ++i) {
        wxPoint pinPos = elem->GetPos() + wxPoint(outputs[i].pos.x, outputs[i].pos.y);
        outputPins.emplace_back(idx, i, PinType::OUTPUT, pinPos);
    }

    // 初始化仿真内部状态为 CanvasElement 的当前输出态（只读）
    outputState = elem->GetOutputState();
}

void SimulationEngine::SetStateChangedCallback(StateChangedCallback cb) {
    m_stateChangedCb = std::move(cb);
}

void SimulationEngine::Initialize(const std::vector<CanvasElement>& elements, const std::vector<Wire>& wires) {
    m_elements.clear();
    m_wires.clear();
    m_eventQueue = std::queue<SimEvent>();
    m_connectionGraph.clear();

    for (size_t i = 0; i < elements.size(); ++i) {
        m_elements.emplace_back(i, &elements[i]);
    }

    for (size_t i = 0; i < wires.size(); ++i) {
        m_wires.emplace_back(i, &wires[i]);
    }

    BuildConnectionGraph();

    std::cout << "SimulationEngine initialized with " << m_elements.size()
        << " elements and " << m_wires.size() << " wires" << std::endl;
}

void SimulationEngine::StartSimulation() {

    std::cout << "SimulationEngine::StartSimulation: m_elements=" << m_elements.size()
        << " m_wires=" << m_wires.size() << std::endl;

    if (m_isRunning) return;
    m_isRunning = true;

    // 初始化输入元件（Pin_Input）的驱动输出：
    // 使用 CanvasElement 的当前输出态作为初始值（不要盲目覆盖）
    for (auto& elem : m_elements) {
        if (elem.element->GetId() == "Pin_Input") {
            // 保持 CanvasElement 当前状态（ElementSimInfo 构造时已读取一次）
            // 但为了保险再同步一次来源数据
            elem.outputState = elem.element->GetOutputState();
            if (m_stateChangedCb) m_stateChangedCb(elem.index, elem.outputState);
            // 将该输出作为 OUTPUT 变化入队以触发传播
            m_eventQueue.emplace(SimEventType::PIN_STATE_CHANGE, elem.index, 0, PinType::OUTPUT, elem.outputState);
        }
    }

    while (!m_eventQueue.empty()) {
        SimEvent event = m_eventQueue.front();
        m_eventQueue.pop();
        ProcessSimulationEvent(event);
    }

    std::cout << "Simulation started" << std::endl;
}

void SimulationEngine::StopSimulation() {
    m_isRunning = false;
    std::cout << "Simulation stopped" << std::endl;
}

void SimulationEngine::ToggleInputPin(size_t elementIndex) {
    // 增加诊断日志，方便定位双击是否到达引擎
    std::cout << "ToggleInputPin requested for index=" << elementIndex
              << " (engine elements=" << m_elements.size() << ")\n";

    if (elementIndex >= m_elements.size()) {
        std::cout << "  ToggleInputPin: index out of range\n";
        return;
    }

    auto& elem = m_elements[elementIndex];
    if (elem.element->GetId() != "Pin_Input") {
        std::cout << "  ToggleInputPin: element is not Pin_Input, id=" << elem.element->GetId() << "\n";
        return;
    }

    int currentState = elem.outputState;
    int newState;

    // Pin_Input 元件在本方案中只有 0/1 两态（1:0, 2:1）
    if (currentState == 1) newState = 2;
    else newState = 1;

    std::cout << "SimulationEngine::ToggleInputPin: elementIndex=" << elementIndex
        << " newState=" << newState << std::endl;

    elem.outputState = newState;

    // 通知 UI（CanvasPanel 在回调中更新 CanvasElement）
    // 在调用回调前后打印，确认回调存在：
    if (m_stateChangedCb) {
        std::cout << "  calling stateChangedCb for index=" << elem.index << std::endl;
        m_stateChangedCb(elem.index, newState);
        std::cout << "  callback returned\n";
    }
    else {
        std::cout << "  no stateChangedCb registered\n";
    }

    // 把这个变化作为 OUTPUT 引脚的变化入队以传播到连接的目标
    m_eventQueue.emplace(SimEventType::PIN_STATE_CHANGE, elementIndex, 0, PinType::OUTPUT, newState);

    // 处理队列（同步处理，保留原来行为）
    while (!m_eventQueue.empty()) {
        SimEvent event = m_eventQueue.front();
        m_eventQueue.pop();
        ProcessSimulationEvent(event);
    }
}

int SimulationEngine::GetPinState(size_t elementIndex, size_t pinIndex, PinType pinType) const {
    if (elementIndex >= m_elements.size()) return 0;

    // 对于输入引脚的读取：查找连接到该输入引脚的驱动端（可能为多个），合并驱动结果
    if (pinType == PinType::INPUT) {
        auto key = std::make_pair(elementIndex, pinIndex);
        auto it = m_connectionGraph.find(key);
        if (it == m_connectionGraph.end()) {
            // 没有连接 -> invalid
            return 0;
        }
        bool anyHigh = false;
        bool anyLow = false;
        for (const auto& neighbor : it->second) {
            size_t nElem = neighbor.first;
            if (nElem < m_elements.size()) {
                int s = m_elements[nElem].outputState;
                if (s == 2) anyHigh = true;
                else if (s == 1) anyLow = true;
            }
        }
        if (anyHigh) return 2;
        if (anyLow) return 1;
        return 0;
    }

    // 输出引脚直接返回该元件的 outputState
    return m_elements[elementIndex].outputState;
}

std::vector<std::vector<int>> SimulationEngine::GetAllElementStates() const {
    std::vector<std::vector<int>> states;
    states.reserve(m_elements.size());

    for (const auto& elem : m_elements) {
        std::vector<int> elemStates;
        if (elem.element->GetId() == "Pin_Input" || elem.element->GetId() == "Pin_Output") {
            elemStates.push_back(elem.outputState);
        }
        states.push_back(std::move(elemStates));
    }

    return states;
}

bool SimulationEngine::HandleDoubleClick(const wxPoint& clickPos) {
    size_t elementIndex;
    if (IsClickOnInteractivePin(clickPos, elementIndex)) {
        ToggleInputPin(elementIndex);
        return true;
    }
    return false;
}

bool SimulationEngine::IsClickOnInteractivePin(const wxPoint& pos, size_t& elementIndex) const {
    for (const auto& elem : m_elements) {
        if (elem.element->GetId() == "Pin_Input") {
            wxPoint pinPos = elem.element->GetPos();
            int distance = std::sqrt(std::pow(pos.x - pinPos.x, 2) + std::pow(pos.y - pinPos.y, 2));
            if (distance <= CLICK_TOLERANCE) {
                elementIndex = elem.index;
                return true;
            }
        }
    }
    return false;
}

void SimulationEngine::UpdateElementState(size_t elementIndex) {
    if (elementIndex >= m_elements.size()) return;
    m_eventQueue.emplace(SimEventType::ELEMENT_EVALUATE, elementIndex, 0, PinType::OUTPUT, 0);

    while (!m_eventQueue.empty()) {
        SimEvent event = m_eventQueue.front();
        m_eventQueue.pop();
        ProcessSimulationEvent(event);
    }
}

void SimulationEngine::BuildConnectionGraph() {
    m_connectionGraph.clear();

    for (size_t wireIdx = 0; wireIdx < m_wires.size(); ++wireIdx) {
        const auto& wire = m_wires[wireIdx];
        if (!wire.wire) continue;
        if (wire.wire->pts.size() < 2) continue;

        const auto& startPt = wire.wire->pts.front();
        const auto& endPt = wire.wire->pts.back();

        size_t startElement = (size_t)-1, startPin = (size_t)-1;
        size_t endElement = (size_t)-1, endPin = (size_t)-1;

        for (auto& elem : m_elements) {
            for (size_t i = 0; i < elem.inputPins.size(); ++i) {
                if (CheckPinConnection(elem.inputPins[i].position, startPt.pos)) {
                    startElement = elem.index;
                    startPin = i;
                    elem.inputPins[i].connectedWires.insert({ wireIdx, 0 });
                    break;
                }
            }
            for (size_t i = 0; i < elem.outputPins.size(); ++i) {
                if (CheckPinConnection(elem.outputPins[i].position, startPt.pos)) {
                    startElement = elem.index;
                    startPin = i;
                    elem.outputPins[i].connectedWires.insert({ wireIdx, 0 });
                    break;
                }
            }
        }

        for (auto& elem : m_elements) {
            for (size_t i = 0; i < elem.inputPins.size(); ++i) {
                if (CheckPinConnection(elem.inputPins[i].position, endPt.pos)) {
                    endElement = elem.index;
                    endPin = i;
                    elem.inputPins[i].connectedWires.insert({ wireIdx, wire.wire->pts.size() - 1 });
                    break;
                }
            }
            for (size_t i = 0; i < elem.outputPins.size(); ++i) {
                if (CheckPinConnection(elem.outputPins[i].position, endPt.pos)) {
                    endElement = elem.index;
                    endPin = i;
                    elem.outputPins[i].connectedWires.insert({ wireIdx, wire.wire->pts.size() - 1 });
                    break;
                }
            }
        }

        if (startElement != (size_t)-1 && endElement != (size_t)-1) {
            // 跳过直接把两个输出短接的情形（避免驱动冲突/振荡）
            PinType startType = GetElementPinType(*m_elements[startElement].element, startPin);
            PinType endType = GetElementPinType(*m_elements[endElement].element, endPin);
            if (startType == PinType::OUTPUT && endType == PinType::OUTPUT) {
                m_wires[wireIdx].isConnected = false;
                continue;
            }

            auto startKey = std::make_pair(startElement, startPin);
            auto endKey = std::make_pair(endElement, endPin);

            m_connectionGraph[startKey].push_back(endKey);
            m_connectionGraph[endKey].push_back(startKey);

            m_wires[wireIdx].startConnection = startKey;
            m_wires[wireIdx].endConnection = endKey;
            m_wires[wireIdx].isConnected = true;
        }
    }
}

void SimulationEngine::ProcessSimulationEvent(const SimEvent& event) {
    switch (event.type) {
    case SimEventType::PIN_STATE_CHANGE:
        PropagatePinStateChange(event.elementIndex, event.pinIndex, event.pinType, event.newState);
        break;
    case SimEventType::ELEMENT_EVALUATE:
        EvaluateElement(event.elementIndex);
        break;
    }
}

void SimulationEngine::EvaluateElement(size_t elementIndex) {
    if (elementIndex >= m_elements.size()) return;
    auto& elem = m_elements[elementIndex];
    elem.needsEvaluation = false;

    int result = EvaluateBasicGate(elem);

    // 仅在该元件有输出引脚且不是输入驱动器时更新 outputState
    if (!elem.outputPins.empty() && elem.element->GetId() != "Pin_Input") {
        int currentState = elem.outputState;
        if (currentState != result) {
            elem.outputState = result;
            // 通知 UI
            if (m_stateChangedCb) m_stateChangedCb(elem.index, result);
            // 广播到连线：把变化作为 OUTPUT 引脚变化传播
            m_eventQueue.emplace(SimEventType::PIN_STATE_CHANGE, elem.index, 0, PinType::OUTPUT, result);
        }
    }
}

void SimulationEngine::PropagatePinStateChange(size_t elementIndex, size_t pinIndex, PinType pinType, int newState) {
    if (elementIndex >= m_elements.size()) return;
    auto& elem = m_elements[elementIndex];

    PinConnection* sourcePin = nullptr;

    if (pinType == PinType::INPUT) {
        for (auto& pin : elem.inputPins) {
            if (pin.pinIndex == pinIndex) {
                sourcePin = &pin;
                break;
            }
        }
    }
    else {
        for (auto& pin : elem.outputPins) {
            if (pin.pinIndex == pinIndex) {
                sourcePin = &pin;
                break;
            }
        }
    }

    if (sourcePin) {
        // PropagateThroughWires 将为每个目标入队 PIN_STATE_CHANGE 和目标的 ELEMENT_EVALUATE
        PropagateThroughWires(*sourcePin, newState);
    }
}

void SimulationEngine::PropagateThroughWires(const PinConnection& sourcePin, int newState) {
    for (const auto& wireInfo : sourcePin.connectedWires) {
        size_t wireIdx = wireInfo.first;
        if (wireIdx >= m_wires.size()) continue;

        const auto& wire = m_wires[wireIdx];
        std::pair<size_t, size_t> targetPin;

        if (wire.startConnection.first == sourcePin.elementIndex &&
            wire.startConnection.second == sourcePin.pinIndex) {
            targetPin = wire.endConnection;
        }
        else {
            targetPin = wire.startConnection;
        }

        if (targetPin.first != (size_t)-1 && targetPin.second != (size_t)-1) {
            PinType targetPinType = GetElementPinType(*m_elements[targetPin.first].element, targetPin.second);

            // 把目标引脚的电平变化入队（目标引脚可能是 INPUT）
            m_eventQueue.emplace(SimEventType::PIN_STATE_CHANGE,
                targetPin.first, targetPin.second, targetPinType, newState);

            // 立即安排评估目标元件（因为该引脚被驱动）
            m_eventQueue.emplace(SimEventType::ELEMENT_EVALUATE, targetPin.first, targetPin.second, PinType::OUTPUT, newState);
        }
    }
}

int SimulationEngine::EvaluateBasicGate(const ElementSimInfo& elemInfo) {
    bool allHigh = true;
    bool anyValid = false;

    for (const auto& inputPin : elemInfo.inputPins) {
        int state = GetPinState(elemInfo.index, inputPin.pinIndex, PinType::INPUT);

        if (state == 2) {
            anyValid = true;
        }
        else if (state == 1) {
            allHigh = false;
            anyValid = true;
        }
    }

    if (!anyValid) {
        return 0;
    }

    return allHigh ? 2 : 1;
}

bool SimulationEngine::CheckPinConnection(const wxPoint& pinPos, const wxPoint& wirePoint) const {
    int distance = std::sqrt(std::pow(pinPos.x - wirePoint.x, 2) + std::pow(pinPos.y - wirePoint.y, 2));
    return distance <= CLICK_TOLERANCE;
}

PinType SimulationEngine::GetElementPinType(const CanvasElement& elem, size_t pinIndex) const {
    if (elem.GetId() == "Pin_Input") return PinType::OUTPUT;   // Pin_Input 元件本身是驱动器（对外输出）
    else if (elem.GetId() == "Pin_Output") return PinType::INPUT; // Pin_Output 元件是被观察端
    if (pinIndex < elem.GetInputPins().size()) return PinType::INPUT;
    else return PinType::OUTPUT;
}