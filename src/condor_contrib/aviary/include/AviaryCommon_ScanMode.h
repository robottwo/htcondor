
          #ifndef AviaryCommon_SCANMODE_H
          #define AviaryCommon_SCANMODE_H
        
      
       /**
        * ScanMode.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.0  Built on : Nov 08, 2012 (09:07:42 EST)
        */

       /**
        *  ScanMode class
        */

        namespace AviaryCommon{
            class ScanMode;
        }
        

        
        #include <axutil_qname.h>
        

        #include <stdio.h>
        #include <OMElement.h>
        #include <ServiceClient.h>
        #include <ADBDefines.h>

namespace AviaryCommon
{
        
        /* Enumeration for this type */
        typedef enum {
            ScanMode_AFTER,
            ScanMode_BEFORE
        } ADBScanModeEnum;
        
        

        class ScanMode {

        private:
             
                axutil_qname_t* qname;
            std::string property_ScanMode;

                
                bool isValidScanMode;
            

        /*** Private methods ***/
          

        bool WSF_CALL
        setScanModeNil();
            



        /******************************* public functions *********************************/

        public:

        /**
         * Constructor for class ScanMode
         */

        ScanMode();

        /**
         * Destructor ScanMode
         */
        ~ScanMode();


       

        /**
         * Constructor for creating ScanMode
         * @param 
         * @param ScanMode std::string
         * @return newly created ScanMode object
         */
        ScanMode(std::string arg_ScanMode);
        ScanMode(const ADBScanModeEnum arg_ScanMode);
        

        /**
         * resetAll for ScanMode
         */
        WSF_EXTERN bool WSF_CALL resetAll();
        
        /********************************** Class get set methods **************************************/
        
        

        /**
         * Getter for ScanMode. 
         * @return std::string*
         */
        WSF_EXTERN std::string WSF_CALL
        getScanMode();

        /**
         * Setter for ScanMode.
         * @param arg_ScanMode std::string*
         * @return true on success, false otherwise
         */
        WSF_EXTERN bool WSF_CALL
        setScanMode(const std::string  arg_ScanMode);

        /**
         * Re setter for ScanMode
         * @return true on success, false
         */
        WSF_EXTERN bool WSF_CALL
        resetScanMode();
        
            
            /************************** Getters and Setters For Enumerations ********************************/
            /********************* Enumeration Specific Operations: get_enum, set_enum **********************/
            
            /**
            * Enum getter for ScanMode.
            * @return ADBScanModeEnum; -1 on failure
            */
            ADBScanModeEnum WSF_CALL
            getScanModeEnum();
            
            /**
            * Enum setter for ScanMode.
            * @param arg_ScanMode ADBScanModeEnum
            * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
            */
            bool WSF_CALL
            setScanModeEnum(
            const ADBScanModeEnum arg_ScanMode);
            
          


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether ScanMode is Nill
         * @return true if the element is Nil, false otherwise
         */
        bool WSF_CALL
        isScanModeNil();


        

        /**************************** Serialize and De serialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Deserialize the content from a string to an ADB object
         * @param node_value to deserialize
         * @param parent_element The parent element if it is an element, NULL otherwise
         * @return true on success, false otherwise
         */
       bool WSF_CALL
       deserializeFromString(const axis2_char_t *node_value, axiom_node_t *parent);
        
        /**
         * Deserialize the ADB object to an XML
         * @param dp_parent double pointer to the parent node to be deserialized
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return true on success, false otherwise
         */
        bool WSF_CALL
        deserialize(axiom_node_t** omNode, bool *isEarlyNodeValid, bool dontCareMinoccurs);
                         
            

       /**
         * Declare namespace in the most parent node 
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
        void WSF_CALL
        declareParentNamespaces(axiom_element_t *parent_element, axutil_hash_t *namespaces, int *next_ns_index);


        
        /**
         * Serialize ADB object to a string
         * @param namespaces hash which contains a mapping of namespace uris to prefixes
         * @return serialized string
         */
         char* WSF_CALL
         serializeToString(axutil_hash_t *namespaces);
        

        /**
         * Serialize the ADB object to an xml
         * @param ScanMode_om_node node to serialize from
         * @param ScanMode_om_element parent element to serialize from
         * @param tag_closed Whether the parent tag is closed or not
         * @param namespaces hash of namespace uris to prefixes
         * @param next_ns_index an int which contains the next namespace index
         * @return axiom_node_t on success,NULL otherwise.
         */
        axiom_node_t* WSF_CALL
        serialize(axiom_node_t* ScanMode_om_node, axiom_element_t *ScanMode_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the ScanMode is a particle class (E.g. group, inner sequence)
         * @return true if this is a particle class, false otherwise.
         */
        bool WSF_CALL
        isParticle();



        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

      
        

        /**
         * Getter for ScanMode by property number (1)
         * @return std::string
         */

        std::string WSF_CALL
        getProperty1();

    

};

}        
 #endif /* SCANMODE_H */
    

