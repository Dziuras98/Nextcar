#include "../include/cylinder_head.h"

#include "../include/cylinder_bank.h"
#include "../include/valvetrain.h"

#include <assert.h>

CylinderHead::CylinderHead() {
    m_cylinders = nullptr;
    m_flipDisplay = false;
    m_bank = nullptr;
    m_valvetrain = nullptr;
    m_exhaustPortFlow = nullptr;
    m_intakePortFlow = nullptr;
    m_intakeRunnerVolume = 0.0;
    m_intakeRunnerCrossSectionArea = 0.0;
    m_exhaustRunnerVolume = 0.0;
    m_exhaustRunnerCrossSectionArea = 0.0;
    m_combustionChamberVolume = 0.0;
}

CylinderHead::~CylinderHead() { /* void */ }

void CylinderHead::initialize(const Parameters &params) {
    m_cylinders = new Cylinder[params.bank->getCylinderCount()];
    m_bank = params.bank;
    m_valvetrain = params.valvetrain;
    m_exhaustPortFlow = params.exhaustPortFlow;
    m_intakePortFlow = params.intakePortFlow;
    m_combustionChamberVolume = params.combustionChamberVolume;
    m_flipDisplay = params.flipDisplay;
    m_intakeRunnerVolume = params.intakeRunnerVolume;
    m_intakeRunnerCrossSectionArea = params.intakeRunnerCrossSectionArea;
    m_exhaustRunnerVolume = params.exhaustRunnerVolume;
    m_exhaustRunnerCrossSectionArea = params.exhaustRunnerCrossSectionArea;
}

void CylinderHead::destroy() {
    if (m_cylinders != nullptr) delete[] m_cylinders;
    m_cylinders = nullptr;
}

double CylinderHead::intakeFlowRate(int cylinder) const {
    return m_intakePortFlow->sampleTriangle(intakeValveLift(cylinder));
}

double CylinderHead::exhaustFlowRate(int cylinder) const {
    return m_exhaustPortFlow->sampleTriangle(exhaustValveLift(cylinder));
}

double CylinderHead::intakeValveLift(int cylinder) const { return m_valvetrain->intakeValveLift(cylinder); }
double CylinderHead::exhaustValveLift(int cylinder) const { return m_valvetrain->exhaustValveLift(cylinder); }

void CylinderHead::setAllExhaustSystems(ExhaustSystem *system) {
    for (int i = 0; i < m_bank->getCylinderCount(); ++i) m_cylinders[i].exhaustSystem = system;
}
void CylinderHead::setExhaustSystem(int i, ExhaustSystem *system) { m_cylinders[i].exhaustSystem = system; }
void CylinderHead::setSoundAttenuation(int i, double soundAttenuation) { m_cylinders[i].soundAttenuation = soundAttenuation; }
void CylinderHead::setAllIntakes(Intake *intake) {
    for (int i = 0; i < m_bank->getCylinderCount(); ++i) m_cylinders[i].intake = intake;
}
void CylinderHead::setIntake(int i, Intake *intake) { m_cylinders[i].intake = intake; }
void CylinderHead::setAllHeaderPrimaryLengths(double length) {
    for (int i = 0; i < m_bank->getCylinderCount(); ++i) m_cylinders[i].headerPrimaryLength = length;
}
void CylinderHead::setHeaderPrimaryLength(int i, double length) { m_cylinders[i].headerPrimaryLength = length; }
Camshaft *CylinderHead::getExhaustCamshaft() { return m_valvetrain->getActiveExhaustCamshaft(); }
Camshaft *CylinderHead::getIntakeCamshaft() { return m_valvetrain->getActiveIntakeCamshaft(); }
