#ifndef _H_TransactionInterpreter
#define _H_TransactionInterpreter

#include "DbClientTransactionPlayer.hh"
#include "ConstrainedVariable.hh"
#include "IntervalToken.hh"
#include "Object.hh"
#include "ObjectFactory.hh"
#include "PlanDatabaseDefs.hh"
#include "Rule.hh"
#include "RuleInstance.hh"
#include "RulesEngineDefs.hh"
#include "Timeline.hh"
#include "TokenFactory.hh"
#include "Debug.hh"
#include <map>
#include <vector>


namespace EUROPA {

  class Expr;
  
  class InterpretedDbClientTransactionPlayer : public DbClientTransactionPlayer {
    public:
      InterpretedDbClientTransactionPlayer(const DbClientId & client);
      virtual ~InterpretedDbClientTransactionPlayer();

    protected:
      virtual void playDeclareClass(const TiXmlElement &); 
      virtual void playDefineClass(const TiXmlElement &); 
      virtual void playDefineCompat(const TiXmlElement &);
      virtual void playDefineEnumeration(const TiXmlElement &);
      virtual void playDefineType(const TiXmlElement &);
      
      void defineClassMember(Id<Schema>& schema, const char* className,  const TiXmlElement* element);
      int  defineConstructor(Id<Schema>& schema, const char* className,  const TiXmlElement* element);
      void declarePredicate(Id<Schema>& schema, const char* className,  const TiXmlElement* element);
      void defineEnum(Id<Schema>& schema, const char* className,  const TiXmlElement* element);
      
      Expr* valueToExpr(const TiXmlElement* element);
      
      // TODO: move these to schema
      std::set<std::string> m_systemClasses;      
      std::set<std::string> m_systemTokens;      
  };
  
  class DataRef
  {
  	public :
  	    DataRef();
  	    DataRef(const ConstrainedVariableId& v);
  	    virtual ~DataRef();
  	    
  	    ConstrainedVariableId& getValue();
  	    
  	    static DataRef null;

  	protected :  	    
  	    ConstrainedVariableId m_value;
  };
  
  class EvalContext 
  {
  	public:
  	    EvalContext(EvalContext* parent); 
  	    virtual ~EvalContext();
  	    
  	    virtual void addVar(const char* name,const ConstrainedVariableId& v); 
  	    virtual ConstrainedVariableId getVar(const char* name);
  	
  	    virtual void addToken(const char* name,const TokenId& t); 
  	    virtual TokenId getToken(const char* name);

        virtual std::string EvalContext::toString() const;
  	    
  	protected:
  	    EvalContext* m_parent;      	    
  	    std::map<std::string,ConstrainedVariableId> m_variables;
  	    std::map<std::string,TokenId> m_tokens;
  };
  
  class Expr
  {      	
  	public:
  	    virtual DataRef eval(EvalContext& context) const = 0;
  };
  
  // Call to super inside a constructor 
  class ExprConstructorSuperCall : public Expr
  {      	
  	public:
  	    ExprConstructorSuperCall(const LabelStr& superClassName, 
  	                             const std::vector<Expr*>& argExprs);
  	    virtual ~ExprConstructorSuperCall();
  	    
  	    virtual DataRef eval(EvalContext& context) const;
  	    
  	    const LabelStr& getSuperClassName() const { return m_superClassName; }
  	    
  	    void evalArgs(EvalContext& context, std::vector<const AbstractDomain*>& arguments) const;
  	    
  	protected:
  	    LabelStr m_superClassName;
        std::vector<Expr*> m_argExprs;	                                	      	    
  };
  
  // Assignment inside a constructor 
  // TODO: make lhs an Expr as well to make this a generic assignment
  class ExprConstructorAssignment : public Expr
  {      	
  	public:
  	    ExprConstructorAssignment(const char* lhs, 
  	                              Expr* rhs);
  	    virtual ~ExprConstructorAssignment();
  	    
  	    virtual DataRef eval(EvalContext& context) const;
  	    
    protected:
        const char* m_lhs;
        Expr* m_rhs;
  };
  
  class ExprConstant : public Expr
  {
  	public:
  	    ExprConstant(DbClientId& dbClient, const char* type, const AbstractDomain* d);
  	    virtual ~ExprConstant();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
  	    ConstrainedVariableId m_var;
  };
  
  class ExprVariableRef : public Expr
  {
  	public:
  	    ExprVariableRef(const char* name);
  	    virtual ~ExprVariableRef();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
  	    LabelStr m_varName;    	    
  };
  
  class ExprNewObject : public Expr
  {
  	public:
  	    ExprNewObject(const DbClientId& dbClient,
	                  const LabelStr& objectType, 
	                  const LabelStr& objectName,
	                  const std::vector<Expr*>& argExprs);
	                  
	    virtual ~ExprNewObject();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
        DbClientId            m_dbClient;
	    LabelStr              m_objectType; 
	    LabelStr              m_objectName;
	    std::vector<Expr*>    m_argExprs;  
  };
    
  class InterpretedObjectFactory : public ConcreteObjectFactory
  {
  	public:
  	    InterpretedObjectFactory(
  	        const char* className,
  	        const LabelStr& signature, 
  	        const std::vector<std::string>& constructorArgNames,
  	        const std::vector<std::string>& constructorArgTypes,
  	        ExprConstructorSuperCall* superCallExpr,
  	        const std::vector<Expr*>& constructorBody,
  	        bool canMakeNewObject = false
  	    );
  	    
  	    virtual ~InterpretedObjectFactory();
  	      	    
	protected:
	    // createInstance = makeNewObject + evalConstructorBody
	    virtual ObjectId createInstance(const PlanDatabaseId& planDb,
	                            const LabelStr& objectType, 
	                            const LabelStr& objectName,
	                            const std::vector<const AbstractDomain*>& arguments) const;
	                             
        // Any exported C++ classes must register a factory for each C++ constructor 
        // and override this method to call the C++ constructor 
    	virtual ObjectId makeNewObject( 
	                        const PlanDatabaseId& planDb,
	                        const LabelStr& objectType, 
	                        const LabelStr& objectName,
	                        const std::vector<const AbstractDomain*>& arguments) const;
	                        
	    virtual void evalConstructorBody(
	                       DbClientId dbClient,
	                       ObjectId& instance, 
	                       const std::vector<const AbstractDomain*>& arguments) const;
  
	    bool checkArgs(const std::vector<const AbstractDomain*>& arguments) const;

        LabelStr m_className;
        std::vector<std::string>  m_constructorArgNames;	                          
        std::vector<std::string>  m_constructorArgTypes;	
        ExprConstructorSuperCall* m_superCallExpr;                          
        std::vector<Expr*>        m_constructorBody;
        bool                      m_canMakeNewObject;	 
  };  
  
  // InterpretedToken is the interpreted version of NddlToken
  class InterpretedToken : public IntervalToken
  {
  	public:
  	    // Same Constructor signatures as NddlToken, see if both are needed
  	    InterpretedToken(const PlanDatabaseId& planDatabase, 
  	                     const LabelStr& predicateName, 
                         const std::vector<LabelStr>& parameterNames,
                         const std::vector<LabelStr>& parameterTypes,
	                     const std::vector<LabelStr>& assignVars,
                         const std::vector<Expr*>& assignValues,
                         const bool& rejectable = false, 
  	                     const bool& close = false); 
  	                     
        InterpretedToken(const TokenId& master, 
                         const LabelStr& predicateName, 
                         const LabelStr& relation, 
                         const std::vector<LabelStr>& parameterNames,
                         const std::vector<LabelStr>& parameterTypes,
	                     const std::vector<LabelStr>& assignVars,
                         const std::vector<Expr*>& assignValues,
                         const bool& close = false); 
                         
        
  	    virtual ~InterpretedToken();

    protected:
        void InterpretedToken::commonInit(const std::vector<LabelStr>& parameterNames,
                                          const std::vector<LabelStr>& parameterTypes,
                                          const std::vector<LabelStr>& assignVars,
                                          const std::vector<Expr*>& assignValues,
                                          const bool& autoClose);      	                                          
  };
  
  class InterpretedTokenFactory: public ConcreteTokenFactory 
  { 
    public: 
	  InterpretedTokenFactory(const LabelStr& predicateName,
	                          const std::vector<LabelStr>& parameterNames,
                              const std::vector<LabelStr>& parameterTypes,
	                          const std::vector<LabelStr>& assignVars,
                              const std::vector<Expr*>& assignValues);
	  
	protected:
	  std::vector<LabelStr> m_parameterNames;    
	  std::vector<LabelStr> m_parameterTypes;    
	  std::vector<LabelStr> m_assignVars;    
	  std::vector<Expr*> m_assignValues;    

	private: 
	  virtual TokenId createInstance(const PlanDatabaseId& planDb, const LabelStr& name, bool rejectable = false) const;
	  virtual TokenId createInstance(const TokenId& master, const LabelStr& name, const LabelStr& relation) const;
  };

  class RuleExpr;
  
  class InterpretedRuleInstance : public RuleInstance
  {
  	public:
  	    InterpretedRuleInstance(const RuleId& rule, 
  	                            const TokenId& token, 
  	                            const PlanDatabaseId& planDb,
                                const std::vector<RuleExpr*>& body);
  	    virtual ~InterpretedRuleInstance();
  	    
        void createConstraint(const LabelStr& name, std::vector<ConstrainedVariableId>& vars);

        TokenId createSubgoal(
                   const LabelStr& name,
                   const LabelStr& predicateType, 
                   const LabelStr& predicateInstance, 
                   const LabelStr& relation);
                   
        // TODO: this should eventually replace addVariable in RuleInstance    
        // it's a dynamic version that doesn't require knowing the actual baseDomain of the variable at compile time               
        ConstrainedVariableId addLocalVariable( 
                       const AbstractDomain& baseDomain,
				       bool canBeSpecified,
				       const LabelStr& name);                   

    protected:
        std::vector<RuleExpr*> m_body;                                          

        /**
         * @brief provide implementation for this method for firing the rule
         */
        virtual void handleExecute();
  };
  
  class RuleInstanceEvalContext : public EvalContext
  {
  	public:
  	    RuleInstanceEvalContext(EvalContext* parent, const RuleInstanceId& ruleInstance);
  	    virtual ~RuleInstanceEvalContext();   	
  	    
  	    virtual ConstrainedVariableId getVar(const char* name);  
  	    
  	protected:
  	    RuleInstanceId m_ruleInstance;	    
  };
  
  class InterpretedRuleFactory : public Rule
  {
    public:
        InterpretedRuleFactory(const LabelStr& predicate, const LabelStr& source, const std::vector<RuleExpr*>& ruleBody); 
        virtual ~InterpretedRuleFactory();
        
        virtual RuleInstanceId createInstance(const TokenId& token, 
                                              const PlanDatabaseId& planDb, 
                                              const RulesEngineId &rulesEngine) const;
                                              
    protected:
        std::vector<RuleExpr*> m_body;                                                                                            
  };   
  
  /*
   * Expr that appears in the body of an interpreted rule instance
   * 
   */
  class RuleExpr  : public Expr
  {
  	protected:
  	    friend class InterpretedRuleInstance;
  	    
  	    InterpretedRuleInstance* m_ruleInstance;
  	    
  	    void setRuleInstance(InterpretedRuleInstance* ri) { m_ruleInstance = ri; }  	    
  };
  
  class ExprConstraint : public RuleExpr
  {
  	public:
  	    ExprConstraint(const char* name,const std::vector<Expr*> args);
  	    virtual ~ExprConstraint();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
  	    LabelStr m_name;
  	    std::vector<Expr*> m_args;    	    
  };
  
  class ExprSubgoal : public RuleExpr
  {
  	public:
  	    ExprSubgoal(const char* name,
  	                const char* predicateType,
  	                const char* predicateInstance,
  	                const char* relation);
  	    virtual ~ExprSubgoal();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
  	    LabelStr m_name;
  	    LabelStr m_predicateType;
  	    LabelStr m_predicateInstance;
  	    LabelStr m_relation;
  };
  
  class ExprLocalVar : public RuleExpr
  {
  	public:
  	    ExprLocalVar(const LabelStr& name,
  	                 const LabelStr& type);
  	    virtual ~ExprLocalVar();

  	    virtual DataRef eval(EvalContext& context) const;  
  	    
  	protected:
  	    LabelStr m_name;
  	    LabelStr m_type;
  	    const AbstractDomain* m_baseDomain;
  };      
  
  class ExprIf : public RuleExpr
  {
  	public:
  	    ExprIf();
  	    virtual ~ExprIf();

  	    virtual DataRef eval(EvalContext& context) const;  
  };        
  
  
  // TODO: create a separate file for exported C++ classes?
  class NativeObjectFactory : public InterpretedObjectFactory
  {
  	public:
  	    NativeObjectFactory(const char* className, const LabelStr& signature) 
  	        : InterpretedObjectFactory(
  	              className,                  // className
  	              signature,                  // signature
  	              std::vector<std::string>(), // ConstructorArgNames
  	              std::vector<std::string>(), // constructorArgTypes
  	              NULL,                       // SuperCallExpr
  	              std::vector<Expr*>(),       // constructorBody
  	              true                        // canCreateObjects
  	          )
  	    {
  	    }
  	    
  	    virtual ~NativeObjectFactory() {}
  	    
  	protected:
    	virtual ObjectId makeNewObject( 
	                        const PlanDatabaseId& planDb,
	                        const LabelStr& objectType, 
	                        const LabelStr& objectName,
	                        const std::vector<const AbstractDomain*>& arguments) const = 0;
  };
  
  class NativeTokenFactory: public ConcreteTokenFactory 
  { 
    public: 
	  NativeTokenFactory(const LabelStr& predicateName) : ConcreteTokenFactory(predicateName) {}
	  
	private: 
	  virtual TokenId createInstance(const PlanDatabaseId& planDb, const LabelStr& name, bool rejectable = false) const = 0;
	  virtual TokenId createInstance(const TokenId& master, const LabelStr& name, const LabelStr& relation) const = 0;
  };  
  
  class TimelineObjectFactory : public NativeObjectFactory 
  { 
  	public: 
  	    TimelineObjectFactory(const LabelStr& signature);
  	    virtual ~TimelineObjectFactory(); 
  	
  	protected: 
    	virtual ObjectId makeNewObject( 
	                        const PlanDatabaseId& planDb, 
	                        const LabelStr& objectType, 
	                        const LabelStr& objectName, 
	                        const std::vector<const AbstractDomain*>& arguments) const;
  }; 
  
  class ReusableObjectFactory : public NativeObjectFactory 
  { 
  	public: 
  	    ReusableObjectFactory(const LabelStr& signature);
  	    virtual ~ReusableObjectFactory(); 
  	
  	protected: 
    	virtual ObjectId makeNewObject( 
	                        const PlanDatabaseId& planDb, 
	                        const LabelStr& objectType, 
	                        const LabelStr& objectName, 
	                        const std::vector<const AbstractDomain*>& arguments) const;
  };   
  
  class ReusableUsesTokenFactory: public NativeTokenFactory 
  { 
    public: 
	  ReusableUsesTokenFactory(const LabelStr& predicateName) : NativeTokenFactory(predicateName) {}
	  
	private: 
	  virtual TokenId createInstance(const PlanDatabaseId& planDb, const LabelStr& name, bool rejectable = false) const;
	  virtual TokenId createInstance(const TokenId& master, const LabelStr& name, const LabelStr& relation) const;
  };   
}

#endif // _H_TransactionInterpreter
