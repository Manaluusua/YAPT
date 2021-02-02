#pragma once
#include <qspinbox.h>
#include <type_traits>
#include <qlayout.h>


namespace YAPT
{

	class MutableNumericElementListener
	{
	public:
		virtual void numericElementValueChanged() = 0;
	};

	class IntegralSpinBoxCallbackHelper : public QObject
	{
		Q_OBJECT
	public:
		void setup(QSpinBox* spinBox, MutableNumericElementListener* listener)
		{
			m_listener = listener;

			void (QSpinBox:: * valueChangedFunc)(int) = &QSpinBox::valueChanged;
			connect(spinBox, valueChangedFunc, this, &IntegralSpinBoxCallbackHelper::valueChanged);
		}

		void valueChanged(int v)
		{
			if (m_listener)
			{
				m_listener->numericElementValueChanged();
			}
		}

	private:
		MutableNumericElementListener* m_listener;
	};

	class FloatSpinBoxCallbackHelper : public QObject
	{
		Q_OBJECT
	public:
		void setup(QDoubleSpinBox* spinBox, MutableNumericElementListener* listener)
		{
			m_listener = listener;

			void (QDoubleSpinBox:: * valueChangedFunc)(double) = &QDoubleSpinBox::valueChanged;
			connect(spinBox, valueChangedFunc, this, &FloatSpinBoxCallbackHelper::valueChanged);
		}

		void valueChanged(double v)
		{
			if (m_listener)
			{
				m_listener->numericElementValueChanged();
			}
		}

	private:
		MutableNumericElementListener* m_listener;
	};

	namespace Detail
	{
		template<typename T, bool = std::is_integral<T>::value>
		struct SpinBoxSelector
		{

		};

		template<typename T>
		struct SpinBoxSelector<T, true>
		{
			typedef typename QSpinBox SpinBoxType;
			typedef typename IntegralSpinBoxCallbackHelper SpinBoxCallbackHelperType;
		};

		template<typename T>
		struct SpinBoxSelector<T, false>
		{
			typedef typename QDoubleSpinBox SpinBoxType;
			typedef typename FloatSpinBoxCallbackHelper SpinBoxCallbackHelperType;
			
		};
	}

	template<typename T>
	class MutableNumericElement : public QWidget
	{
		typedef typename Detail::SpinBoxSelector<T>::SpinBoxType SpinBox;
		typedef typename Detail::SpinBoxSelector<T>::SpinBoxCallbackHelperType  CallbackHelper;


	public:
		explicit MutableNumericElement(QWidget* parent, MutableNumericElementListener* changedCallback)
			:QWidget(parent)
		{
			m_layout = new QHBoxLayout(this);
			m_spinBox = new SpinBox(this);
			m_layout->addWidget(m_spinBox);
			m_callbackHelper.setup(m_spinBox, changedCallback);

			m_spinBox->setRange(0, 10e9);

		}
		~MutableNumericElement()
		{

		}
		void setLimits(float min, float max)
		{
			m_spinBox->setRange(min, max);
		}

		void setValue(float val)
		{
			m_spinBox->setValue(val);
		}

		float getValue() const
		{
			return m_spinBox->value();
		}

	private:

		void valueChanged(float v)
		{

		}
		CallbackHelper m_callbackHelper;
		QHBoxLayout* m_layout;
		SpinBox* m_spinBox;

	};

	/*
	class MutableNumericElementFloat : public QWidget
	{
		
	public:
		explicit MutableNumericElementFloat(QWidget* parent)
			:QWidget(parent)
		{
			m_layout = new QHBoxLayout(this);
			m_spinBox = new QDoubleSpinBox(this);
			m_layout->addWidget(m_spinBox);

		}
		~MutableNumericElementFloat()
		{

		}


		void MutableNumericElementFloat::setLimits(float min, float max)
		{
			m_spinBox->setRange(min, max);
		}

		void MutableNumericElementFloat::setValue(float val)
		{
			m_spinBox->setValue(val);
		}

		float MutableNumericElementFloat::getValue() const
		{
			return m_spinBox->value();
		}

	private:
		QHBoxLayout* m_layout;
		QDoubleSpinBox* m_spinBox;

	};*/
	

}

